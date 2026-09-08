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

pub fn splinter_coefficients2d_inplace(
    data: &mut [f64],
    width: i32,
    height: i32,
    ext: BoundaryExtension,
    spline_order: u8,
    epsilon: f64,
) {
    unsafe {
        bindings::splinter_prefilter_inplace2d(
            data.as_mut_ptr(),
            width,
            height,
            ext,
            spline_order,
            epsilon,
        );
    }
}

mod bindings {
    use crate::BoundaryExtension;

    unsafe extern "C" {
        pub fn splinter_expfilter(
            data: *mut f64,
            step: i32,
            n: i32,
            extension: u8,
            alpha: f64,
            n0: i32,
        );
        pub fn splinter_prefilter_inplace2d(
            data: *const f64,
            width: i32,
            height: i32,
            ext: BoundaryExtension,
            spline_order: u8,
            epsilon: f64,
        );
    }
}
