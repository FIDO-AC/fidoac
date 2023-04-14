
//Module
mod zkp;
use crate::zkp::zkpmain::fidoac_prove_main;

use log::*;
// extern crate android_logger;
// use log::LevelFilter;
// use android_logger::{Config,FilterBuilder};

mod common;
// use crate::common;

use std::os::raw::{c_char};
use std::ffi::{CString, CStr};
extern crate jni;

use jni::JNIEnv;
use jni::objects::{JClass, JObject, JValue, JString};
use jni::sys::{jstring, jbyteArray, jbyte, jint, jboolean};
pub type Callback = unsafe extern "C" fn(*const c_char) -> ();

#[no_mangle]
#[allow(non_snake_case)]
pub extern "C" fn invokeCallbackViaJNI(callback: Callback) {
    let s = CString::new("Hello from Rust").unwrap();
    unsafe { callback(s.as_ptr()); }
}

fn  as_vec_u8(env :&JNIEnv,  array:jbyteArray) -> Vec<u8> {
    debug!("Converting JavaByteArray to Vec<u8>-Rust");
    return env.convert_byte_array(array).unwrap();
}
fn  as_jbytearray(env :&JNIEnv, mut array:Vec<u8>) -> jbyteArray {
    let newarray = env.new_byte_array(array.len() as i32).unwrap();

    let array_slice = &mut array[..];
    let array_slice_jbyte = unsafe{ &*( array_slice as *mut [u8] as *mut [i8] ) };
    env.set_byte_array_region(newarray, 0, array_slice_jbyte ).unwrap();

    newarray
}
fn pass_to_caller (env :&JNIEnv, obj: &JObject, proof:Vec<u8>, digest:Vec<u8>){
    debug!("Pass back result.");

    // debug!("Digest from Rust: {:02X?}", &digest);
    env.call_method(*obj, "setDigest", "([B)V",
    &[JValue::from(JObject::from(as_jbytearray(&env, digest)))] ).unwrap();

    // "(Ljava/lang/String;)V" for String,
    env.call_method(*obj, "setProof", "([B)V",
    &[JValue::from(JObject::from(as_jbytearray(&env, proof)))] ).unwrap();

}

use log::LevelFilter;
use android_logger::Config;
#[no_mangle]
#[allow(non_snake_case)]
pub extern "C" fn Java_anon_fidoac_MainActivity_rust_1init(
){
    android_logger::init_once(
        Config::default().with_max_level(LevelFilter::Trace),
    );
    // debug!("Starting program.");
    // info!("Things are going fine.");
    // error!("Something went wrong!");
}
// let s = zkpmain::get_mod_name();
// let response = env.new_string(&s)
//     .expect("Couldn't create java string!");
// env.call_method(callback, "printDebug", 
//                 &[JValue::from(JObject::from(response))]).unwrap();

#[no_mangle]
#[allow(non_snake_case)]
pub extern "C" fn Java_anon_fidoac_MainActivity_rust_1fidoacprove(
    env: JNIEnv,
    _class: JClass,
    callback: JObject,
    provkey: jbyteArray,
    verfkey: jbyteArray,
    dg1: jbyteArray,
    client_nonce: jbyteArray,
    age_gt: jint,
    cur_year: jint
) -> jboolean {
    debug!("Hello from Rust");

    debug!("Proving");
    let (is_valid_proof, proof, randomized_dg1_digest) = fidoac_prove_main(as_vec_u8(&env,provkey),as_vec_u8(&env,verfkey),as_vec_u8(&env,dg1),as_vec_u8(&env, client_nonce),age_gt,cur_year);
    debug!("Done ZKP final cleanup");

    pass_to_caller(&env, &callback,proof, randomized_dg1_digest);
    debug!("Finish FIDOAC-Rust");

    is_valid_proof as u8
}




#[no_mangle]
#[allow(non_snake_case)]
pub extern fn rust_greeting(to: *const c_char) -> *mut c_char {
    let c_str = unsafe { CStr::from_ptr(to) };
    let recipient = match c_str.to_str() {
        Err(_) => "there",
        Ok(string) => string,
    };

    CString::new("Hello ".to_owned() + recipient).unwrap().into_raw()
}
#[no_mangle]
#[allow(non_snake_case)]
pub unsafe extern fn Java_anon_fidoac_MainActivity_greeting(env: JNIEnv, _: JClass, java_pattern: JString) -> jstring {
    // Our Java companion code might pass-in "world" as a string, hence the name.
    let world = rust_greeting(env.get_string(java_pattern).expect("invalid pattern string").as_ptr());
    // Retake pointer so that we can use it below and allow memory to be freed when it goes out of scope.
    let world_ptr = CString::from_raw(world);
    let output = env.new_string(world_ptr.to_str().unwrap()).expect("Couldn't create java string!");

    output.into_inner()
}