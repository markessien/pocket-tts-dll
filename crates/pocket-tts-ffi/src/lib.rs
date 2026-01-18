use pocket_tts::{ModelState, TTSModel};
use std::ffi::CStr;
use std::os::raw::c_char;
use std::ptr;

/// Load the model from a specific directory and variant
/// Returns a pointer to the TTSModel, or NULL on failure
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_load_model(
    model_dir: *const c_char,
    variant: *const c_char,
) -> *mut TTSModel {
    if variant.is_null() || model_dir.is_null() {
        return ptr::null_mut();
    }

    let variant_str = match unsafe { CStr::from_ptr(variant) }.to_str() {
        Ok(s) => s,
        Err(_) => return ptr::null_mut(),
    };

    let model_dir_str = match unsafe { CStr::from_ptr(model_dir) }.to_str() {
        Ok(s) => s,
        Err(_) => return ptr::null_mut(),
    };

    match TTSModel::load_from_dir(model_dir_str, variant_str) {
        Ok(model) => Box::into_raw(Box::new(model)),
        Err(e) => {
            eprintln!("Error loading model from {:?}: {:?}", model_dir_str, e);
            ptr::null_mut()
        }
    }
}

/// Free the TTSModel
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_free_model(model: *mut TTSModel) {
    if !model.is_null() {
        unsafe {
            let _ = Box::from_raw(model);
        }
    }
}

/// Create a voice state from an audio file path
/// Returns a pointer to the ModelState, or NULL on failure
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_get_voice_state(
    model: *mut TTSModel,
    audio_path: *const c_char,
) -> *mut ModelState {
    if model.is_null() || audio_path.is_null() {
        return ptr::null_mut();
    }

    let model = unsafe { &*model };
    let path_str = match unsafe { CStr::from_ptr(audio_path) }.to_str() {
        Ok(s) => s,
        Err(_) => return ptr::null_mut(),
    };

    match model.get_voice_state(path_str) {
        Ok(state) => Box::into_raw(Box::new(state)),
        Err(e) => {
            eprintln!("Error getting voice state: {:?}", e);
            ptr::null_mut()
        }
    }
}

/// Free the ModelState
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_free_voice_state(state: *mut ModelState) {
    if !state.is_null() {
        unsafe {
            let _ = Box::from_raw(state);
        }
    }
}

/// Generate audio from text
/// Returns a pointer to a float array of audio samples, and sets out_len to the number of samples
/// Returns NULL on failure
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_generate(
    model: *mut TTSModel,
    text: *const c_char,
    state: *mut ModelState,
    out_len: *mut usize,
) -> *mut f32 {
    if model.is_null() || text.is_null() || state.is_null() || out_len.is_null() {
        return ptr::null_mut();
    }

    let model = unsafe { &*model };
    let state = unsafe { &*state };
    let text_str = match unsafe { CStr::from_ptr(text) }.to_str() {
        Ok(s) => s,
        Err(_) => return ptr::null_mut(),
    };

    match model.generate(text_str, state) {
        Ok(audio_tensor) => {
            match audio_tensor.flatten_all().and_then(|t| t.to_vec1::<f32>()) {
                Ok(audio_data) => {
                    unsafe {
                        *out_len = audio_data.len();
                    }
                    let mut boxed_slice = audio_data.into_boxed_slice();
                    let ptr = boxed_slice.as_mut_ptr();
                    std::mem::forget(boxed_slice);
                    ptr
                }
                Err(e) => {
                    eprintln!("Error converting audio tensor to vector: {:?}", e);
                    ptr::null_mut()
                }
            }
        }
        Err(e) => {
            eprintln!("Error generating audio: {:?}", e);
            ptr::null_mut()
        }
    }
}

/// Free the audio buffer returned by pocket_tts_generate
#[unsafe(no_mangle)]
pub unsafe extern "C" fn pocket_tts_free_audio(buffer: *mut f32, len: usize) {
    if !buffer.is_null() {
        unsafe {
            let _ = Vec::from_raw_parts(buffer, len, len);
        }
    }
}
