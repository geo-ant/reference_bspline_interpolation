use std::ffi::c_int;

use criterion::{BenchmarkId, Criterion, criterion_group, criterion_main};
use rand::{Rng, SeedableRng, distributions::Uniform, rngs::StdRng};

use benchmarks::{BoundaryExtension, apply_expfilter};

fn bench_expfilter(c: &mut Criterion) {
    let mut group = c.benchmark_group("apply_expfilter");

    // stride = 1, varying number of samples
    let sizes: [c_int; _] = [64, 256, 1024, 4096, 16384, 65535];

    // generate one buffer at the largest size; each benchmark uses a prefix,
    // so every size sees the same data (truncated to the relevant length)
    let max_n = *sizes.iter().max().unwrap() as usize;
    let mut rng = StdRng::seed_from_u64(0x12345678);
    let dist = Uniform::new(-100.0f64, 100.0f64);
    let data: Vec<f64> = (0..max_n).map(|_| rng.sample(dist)).collect();

    // typical smoothing parameter
    let alpha = 0.5;

    for n in sizes {
        let data = &data[..n as usize];

        // n_trunc varies with size: powers of 10 from 1 up to n/10
        // e.g. for n = 1024 -> [1, 10, 100]
        let mut n_trunc = 1 as c_int;
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
                                BoundaryExtension::Hsymmetric,
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

criterion_group!(benches, bench_expfilter);
criterion_main!(benches);
