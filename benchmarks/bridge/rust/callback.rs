#[repr(C)]
pub struct BenchBuffer { data: *mut u8, bits: u32, capacity: u32 }
#[repr(C)]
pub struct BenchContext { mode: i32, accumulator: u64 }

#[no_mangle]
pub unsafe extern "C" fn bench_callback(buffer: *mut BenchBuffer, context: *mut BenchContext) -> i32 {
    match (*context).mode {
        1 => (*context).accumulator += *(*buffer).data as u64 + (*buffer).bits as u64,
        2 => *(*buffer).data ^= 1,
        3 => return 1,
        _ => {}
    }
    0
}

#[no_mangle]
pub extern "C" fn bench_begin() {}
#[no_mangle]
pub extern "C" fn bench_allocations() -> u64 { 0 }
