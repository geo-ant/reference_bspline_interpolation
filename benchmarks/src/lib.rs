use std::ffi::c_int;

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum BoundaryExtension {
    Constant = 0,
    Hsymmetric = 1,
    Wsymmetic = 2,
    Periodic = 3,
}

impl BoundaryExtension {
    fn to_c_int(self) -> c_int {
        self as _
    }
}

#[inline(always)]
pub fn apply_expfilter(
    data: &mut [f64],
    stride: u16,
    count: u16,
    extension: BoundaryExtension,
    alpha: f64,
    n_trunc: u16,
) {
    unsafe {
        bindings::splinter_expfilter(
            data.as_mut_ptr(),
            stride as _,
            count as _,
            extension.to_c_int(),
            alpha,
            n_trunc as _,
        );
    }
}

mod bindings {
    use std::ffi::c_int;
    unsafe extern "C" {
        pub fn splinter_expfilter(
            data: *mut f64,
            step: c_int,
            n: c_int,
            extension: c_int,
            alpha: f64,
            n0: c_int,
        );
    }
}
