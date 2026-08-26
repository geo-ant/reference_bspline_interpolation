use criterion::{BenchmarkId, Criterion, criterion_group, criterion_main};
use interpn::multibspline::regular::{MultiBsplineRegular, coefficients, coefficients_par};
use rand::{Rng, SeedableRng, distributions::Uniform, rngs::StdRng};

use benchmarks::{BoundaryExtension, apply_expfilter};

// stride = 1, varying number of samples
const SIZES: [u32; 6] = [64, 256, 1024, 4096, 16384, 65535];

/// Generate one buffer at the largest size; each benchmark uses a prefix,
/// so every size sees the same data (truncated to the relevant length).
fn make_data() -> Vec<f64> {
    let max_n = *SIZES.iter().max().unwrap() as usize;
    let mut rng = StdRng::seed_from_u64(0x12345678);
    let dist = Uniform::new(-100.0f64, 100.0f64);
    (0..max_n).map(|_| rng.sample(dist)).collect()
}

fn bench_expfilter(c: &mut Criterion) {
    let mut group = c.benchmark_group("apply_expfilter");

    let data = make_data();

    // typical smoothing parameter
    let alpha = 0.5;

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

        group.bench_with_input(BenchmarkId::from_parameter(n), &data, |b, data| {
            // b.iter(|| coefficients(dims, data, &mut coeffs, &mut scratch).unwrap());
            b.iter(|| coefficients_par(dims, data, &mut coeffs, &mut scratch, 4).unwrap());
        });
    }

    group.finish();
}

criterion_group!(benches, bench_expfilter, bench_interpn_coeffs);
criterion_main!(benches);
