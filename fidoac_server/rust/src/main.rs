mod zkp;
use crate::zkp::zkpmain::generate_keys;
use crate::zkp::zkpmain::parse_proof;
use crate::zkp::zkpmain::parse_verf_key;
use crate::zkp::zkpmain::fidoac_verify;
use crate::zkp::zkpmain::ConstraintF;

use std::fs::File;
use ark_serialize::*;

use std::os::raw::{c_char};
use std::ffi::{CString, CStr};

use serde_json::{Result, Value};
use base64::{Engine as _, engine::general_purpose};

const PROOF_ID: &str = "PROOF_ID";
const HASH_ID: &str  = "HASH_ID";
const AGEGT_ID: &str  = "AGEGT_ID";
const CURYEAR_ID: &str  = "CURYEAR_ID";

use std::env;
use base64::{engine, alphabet, Engine as _};


fn parse_fidoac_json(json_string: &str) -> (Vec<u8>,Vec<u8>,u8,u8){
    // Parse the string of data into serde_json::Value.
    let v: Value = serde_json::from_str(json_string).unwrap();
    let proof_bytes = engine::general_purpose::URL_SAFE.decode(v[PROOF_ID].as_str().unwrap()).unwrap();
    let digest_bytes = engine::general_purpose::URL_SAFE.decode(v[HASH_ID].as_str().unwrap()).unwrap();
    let age_gt = v[AGEGT_ID].as_u64().unwrap();
    let cur_year = v[CURYEAR_ID].as_u64().unwrap();

    (proof_bytes, digest_bytes, age_gt as u8, cur_year as u8)
}

fn verify_proof(vk_bytes: Vec<u8>, proof_bytes:Vec<u8>, randomized_digest_hash:Vec<u8>, age_gt:u8, cur_year:u8) -> bool{
    let is_verified = fidoac_verify::<ConstraintF>(parse_verf_key(vk_bytes), parse_proof(proof_bytes), randomized_digest_hash, age_gt, cur_year);
    println!("Verified?: {}", is_verified);
    is_verified
}

// CRS for the zk-snark (pk + vk)
fn trusted_setup(){
    let (pk_raw, vk_raw) = generate_keys();

    let mut proving_key_file = File::create("provingkey").unwrap();
    proving_key_file.write_all(&pk_raw).unwrap();

    let mut verf_key_file = File::create("verificationkey").unwrap();
    verf_key_file.write_all(&vk_raw).unwrap();
}

fn main() {
    println!("FIDOAC Server Main Executable");
    // trusted_setup();
    let args: Vec<String> = env::args().collect();
    if args.len() <3 {
        println!("Please supply the verification_key file for verification. Then Supply json_string");
        return
    }

    let vf_file_name = &args[1];
    let mut vf_file = File::open(vf_file_name).unwrap();
    let mut vk_bytes = Vec::new();
    vf_file.read_to_end(&mut vk_bytes).unwrap();

    let file_name = &args[2];
    println!("Loading File : {}", file_name);
    let mut file = File::open(file_name).unwrap();
    let mut json_string: String = String::new();
    file.read_to_string(&mut json_string).unwrap();
    // let mut json_string = &args[2];
    // println!("{}",json_string);
    let  (proof_bytes, digest_bytes,age_gt,cur_year) = parse_fidoac_json(&json_string);

    let _is_verified = verify_proof(vk_bytes, proof_bytes, digest_bytes, age_gt, cur_year);

}
