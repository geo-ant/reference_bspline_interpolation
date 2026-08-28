#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u8)]
pub enum BoundaryExtension {
    Constant = 0,
    Hsymmetric = 1,
    Wsymmetic = 2,
    Periodic = 3,
}

#[inline(always)]
pub fn apply_expfilter(
    data: &mut [f64],
    stride: i32,
    count: i32,
    extension: BoundaryExtension,
    alpha: f64,
    n_trunc: i32,
) {
    unsafe {
        bindings::splinter_expfilter(
            data.as_mut_ptr(),
            stride,
            count,
            extension as _,
            alpha,
            n_trunc,
        );
    }
}

mod bindings {
    unsafe extern "C" {
        pub fn splinter_expfilter(
            data: *mut f64,
            step: i32,
            n: i32,
            extension: u8,
            alpha: f64,
            n0: i32,
        );
    }
}
