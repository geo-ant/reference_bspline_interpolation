use criterion::{BenchmarkId, Criterion, criterion_group, criterion_main};
use interpn::multibspline::regular::{MultiBsplineRegular, coefficients, coefficients_par};
use rand::{Rng, SeedableRng, distributions::Uniform, rngs::StdRng};
use std::num::NonZero;

use benchmarks::{BoundaryExtension, apply_expfilter, splinter_coefficients2d_inplace};
use spleen::{
    TransmittableBoundaryExtension as SpleenBoundaryExtension, bspline::BSpline3, expf64,
    inplace::coefficients2d,
};

// stride = 1, varying number of samples
const SIZES: [i32; 6] = [64, 256, 1024, 4096, 16384, 65535];

// square image sides (n x n f64s), chosen from this machine's lscpu cache sizes
// (L1d ~32-48KiB, L2 ~2MiB, L3 24MiB) to force L1-, L2-, and L3/RAM-resident runs:
// 32 -> 8KiB (fits L1), 256 -> 512KiB (exceeds L1, fits L2), 1024 -> 8MiB (exceeds L2),
// 4096 -> 128MiB (well beyond L3, mostly RAM-bound)
const IMG_SIZES_2D: [i32; 4] = [32, 256, 1024, 4096];

/// Generate one buffer at the largest size; each benchmark uses a prefix,
/// so every size sees the same data (truncated to the relevant length).
fn make_data() -> Vec<f64> {
    let max_n = *SIZES.iter().max().unwrap() as usize;
    let mut rng = StdRng::seed_from_u64(0x12345678);
    let dist = Uniform::new(-100.0f64, 100.0f64);
    (0..max_n).map(|_| rng.sample(dist)).collect()
}

fn bench_expfilter(c: &mut Criterion) {
    let mut group = c.benchmark_group("splinter_expfilter");

    let data = make_data();

    let alpha = -0.28;

    for n in SIZES {
        let data = &data[..n as usize];

        // n_trunc varies with size: powers of 10 from 1 up to n/10
        // e.g. for n = 1024 -> [1, 10, 100]
        let mut n_trunc = 1;
        while n_trunc * 10 <= n / 10 {
            n_trunc *= 10;
        }
        let mut truncs = Vec::new();
        while n_trunc >= 1 {
            truncs.push(n_trunc);
            n_trunc /= 10;
        }
        truncs.reverse();

        for n_trunc in truncs {
            group.bench_with_input(
                BenchmarkId::from_parameter(format!("{n}/trunc={n_trunc}")),
                &data,
                |b, data| {
                    b.iter_batched(
                        || data.to_vec(),
                        |mut buf| {
                            apply_expfilter(
                                &mut buf,
                                1,
                                n,
                                // just from inspecting the code, this should give the
                                // worst performance since both initialization values
                                // require a for loop to run.
                                // NOTE: measure tho...
                                BoundaryExtension::Periodic,
                                alpha,
                                n_trunc,
                            );
                            buf
                        },
                        criterion::BatchSize::LargeInput,
                    );
                },
            );
        }
    }

    group.finish();
}

/// Benchmark spleen's exposed 1D expfilter (`expf64`), directly comparable to
/// `bench_expfilter` (same sizes, alpha, truncation indices, boundary).
fn bench_spleen_expfilter(c: &mut Criterion) {
    let mut group = c.benchmark_group("spleen_expfilter");

    let data = make_data();

    let alpha = -0.28;

    for n in SIZES {
        let data = &data[..n as usize];

        // n_trunc varies with size: powers of 10 from 1 up to n/10
        // e.g. for n = 1024 -> [1, 10, 100]
        let mut n_trunc = 1;
        while n_trunc * 10 <= n / 10 {
            n_trunc *= 10;
        }
        let mut truncs = Vec::new();
        while n_trunc >= 1 {
            truncs.push(n_trunc);
            n_trunc /= 10;
        }
        truncs.reverse();

        for n_trunc in truncs {
            group.bench_with_input(
                BenchmarkId::from_parameter(format!("{n}/trunc={n_trunc}")),
                &data,
                |b, data| {
                    b.iter_batched(
                        || data.to_vec(),
                        |mut buf| {
                            expf64(
                                alpha,
                                &mut buf,
                                n_trunc as usize,
                                SpleenBoundaryExtension::Periodic,
                            )
                            .unwrap();
                            buf
                        },
                        criterion::BatchSize::LargeInput,
                    );
                },
            );
        }
    }

    group.finish();
}

/// Benchmark interpN's cubic multib-spline coefficient generation.
/// Note: interpN uses natural-spline boundaries (zero 3rd derivative),
/// unlike splinter's half-symmetric expfilter boundaries -- this is a
/// performance reference, not a numerics comparison.
fn bench_interpn_coeffs(c: &mut Criterion) {
    let mut group = c.benchmark_group("interpn_cubic_coeffs");

    let data = make_data();

    for n in SIZES {
        let n = n as usize;
        let data = &data[..n];
        let dims = [n];

        // buffers are write-only for `coefficients`, so they can be reused
        // across timed iterations without any per-iteration setup
        let mut coeffs = vec![0.0; MultiBsplineRegular::<f64, 1>::coeff_storage_len(dims)];
        let mut scratch = vec![0.0; MultiBsplineRegular::<f64, 1>::construction_scratch_len(dims)];

        group.bench_with_input(BenchmarkId::from_parameter(n), data, |b, data| {
            b.iter(|| {
                coefficients(dims, data, &mut coeffs, &mut scratch).unwrap();
                // guard against DCE, consistent with the other coeffs benches
                criterion::black_box(&coeffs);
            });
        });
    }

    group.finish();
}

/// Benchmark splinter's exposed order-3 2D prefilter binding on random square
/// images, each generated fresh per size to avoid cross-size cache warmup.
fn bench_splinter_coeffs2d(c: &mut Criterion) {
    let mut group = c.benchmark_group("splinter_coeffs2d");

    let mut rng = StdRng::seed_from_u64(0x87654321);
    let dist = Uniform::new(-100.0f64, 100.0f64);

    for n in IMG_SIZES_2D {
        let image: Vec<f64> = (0..(n as usize * n as usize))
            .map(|_| rng.sample(dist))
            .collect();

        group.bench_with_input(
            BenchmarkId::from_parameter(format!("{n}x{n}")),
            &image,
            |b, image| {
                b.iter_batched(
                    || image.clone(),
                    |mut buf| {
                        splinter_coefficients2d_inplace(
                            &mut buf,
                            n,
                            n,
                            BoundaryExtension::Periodic,
                            3,
                            1e-6,
                        );
                        buf
                    },
                    criterion::BatchSize::LargeInput,
                );
            },
        );
    }

    group.finish();
}

/// Benchmark interpN's 2D cubic multi-bspline coefficient generation (no
/// evaluation/interpolation). The coeffs out-buffer and scratch workspace are
/// allocated once outside the timed closure; each iteration force-uses coeffs
/// via black_box so the call isn't optimized away.
fn bench_interpn_coeffs2d(c: &mut Criterion) {
    let mut group = c.benchmark_group("interpn_coeffs2d");

    let mut rng = StdRng::seed_from_u64(0xabcdef01);
    let dist = Uniform::new(-100.0f64, 100.0f64);

    for n in IMG_SIZES_2D {
        let dims = [n as usize, n as usize];
        let image: Vec<f64> = (0..(n as usize * n as usize))
            .map(|_| rng.sample(dist))
            .collect();

        let mut coeffs = vec![0.0; MultiBsplineRegular::<f64, 2>::coeff_storage_len(dims)];
        let mut scratch = vec![0.0; MultiBsplineRegular::<f64, 2>::construction_scratch_len(dims)];

        group.bench_with_input(
            BenchmarkId::from_parameter(format!("{n}x{n}")),
            &image,
            |b, image| {
                b.iter(|| {
                    coefficients(dims, image, &mut coeffs, &mut scratch).unwrap();
                    // black_box: a reference can't escape an FnMut closure, so
                    // force-use it here instead of returning it
                    criterion::black_box(&coeffs);
                });
            },
        );
    }

    group.finish();
}

/// Benchmark spleen's in-place 2D cubic coefficient prefilter (no scratch or
/// out-buffer needed, like the splinter binding), for direct comparison with
/// `bench_splinter_coeffs2d` and `bench_interpn_coeffs2d`.
fn bench_spleen_coeffs2d(c: &mut Criterion) {
    let mut group = c.benchmark_group("spleen_coeffs2d");

    let mut rng = StdRng::seed_from_u64(0xdeadbeef);
    let dist = Uniform::new(-100.0f64, 100.0f64);

    for n in IMG_SIZES_2D {
        let image: Vec<f64> = (0..(n as usize * n as usize))
            .map(|_| rng.sample(dist))
            .collect();

        group.bench_with_input(
            BenchmarkId::from_parameter(format!("{n}x{n}")),
            &image,
            |b, image| {
                b.iter_batched(
                    || image.clone(),
                    |mut buf| {
                        coefficients2d(
                            BSpline3::<f64>::default(),
                            &mut buf,
                            n as usize,
                            n as usize,
                            1e-6,
                            SpleenBoundaryExtension::Periodic,
                        )
                        .unwrap();
                        buf
                    },
                    criterion::BatchSize::LargeInput,
                );
            },
        );
    }

    group.finish();
}

/// Benchmark splinter's C expfilter vs spleen's `expf64_strided` directly on
/// real strided (column, stride=width) access, applied to every column of a
/// fresh random image each iteration -- isolates the strided-access code path
/// itself (both are the actual production implementations, not reimplementations).
fn bench_strided_expfilter(c: &mut Criterion) {
    let mut group = c.benchmark_group("strided_expfilter");

    let alpha = -0.28;
    let n_trunc = 10;

    for n in IMG_SIZES_2D {
        let width = n as usize;
        let height = n as usize;

        let mut rng = StdRng::seed_from_u64(0x5555_aaaa);
        let dist = Uniform::new(-100.0f64, 100.0f64);
        let image: Vec<f64> = (0..(width * height)).map(|_| rng.sample(dist)).collect();

        group.bench_with_input(
            BenchmarkId::new("splinter", format!("{n}x{n}")),
            &image,
            |b, image| {
                b.iter_batched(
                    || image.clone(),
                    |mut buf| {
                        for col in 0..width {
                            apply_expfilter(
                                &mut buf[col..],
                                width as i32,
                                height as i32,
                                BoundaryExtension::Periodic,
                                alpha,
                                n_trunc,
                            );
                        }
                        buf
                    },
                    criterion::BatchSize::LargeInput,
                );
            },
        );

        // group.bench_with_input(
        //     BenchmarkId::new("spleen", format!("{n}x{n}")),
        //     &image,
        //     |b, image| {
        //         b.iter_batched(
        //             || image.clone(),
        //             |mut buf| {
        //                 for col in 0..width {
        //                     expf64(
        //                         alpha,
        //                         &mut buf[col..],
        //                         NonZero::new(width).unwrap(),
        //                         NonZero::new(height).unwrap(),
        //                         n_trunc as usize,
        //                         SpleenBoundaryExtension::Periodic,
        //                     )
        //                     .unwrap();
        //                 }
        //                 buf
        //             },
        //             criterion::BatchSize::LargeInput,
        //         );
        //     },
        // );
    }

    group.finish();
}

// 2D benches on hold while we investigate the 1D expfilter gap first
criterion_group!(
    benches,
    // bench_expfilter,
    // bench_spleen_expfilter,
    // bench_strided_expfilter,
    // bench_interpn_coeffs,
    bench_splinter_coeffs2d,
    bench_spleen_coeffs2d,
    bench_interpn_coeffs2d,
);
criterion_main!(benches);
