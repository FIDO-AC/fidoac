
use digest::consts::False;
use log::*;


pub fn get_mod_name() -> String {
  "zkpmain".to_string()
}


pub type ConstraintF = ark_bls12_381::Fr;
use ark_bls12_381::{Bls12_381, Fr};

use std::ops::{Add, Mul, Sub};
use std::time::{Duration, Instant};

// FftParameters
use ark_ff::{
    biginteger::BigInteger, One, PrimeField, ToConstraintField, UniformRand,
};
use ark_groth16::{Groth16, PreparedVerifyingKey, Proof, VerifyingKey, ProvingKey}; //constraints::*
// use ark_r1cs_std::prelude::*;
use ark_relations::{r1cs::{Namespace, ConstraintSystem, ConstraintSynthesizer, ConstraintSystemRef, SynthesisError}, ns};
use ark_crypto_primitives::crh::{CRHSchemeGadget, CRHScheme, sha256::{constraints::{UnitVar,Sha256Gadget,DigestVar}, Sha256, digest::Digest} };
use ark_std::{vec, vec::Vec, rand::RngCore, cmp::Ordering};
// use ark_ec::PairingEngine;
use ark_ec::{pairing::Pairing, AffineRepr, CurveGroup};
use ark_r1cs_std::{R1CSVar,uint8::UInt8};
use ark_r1cs_std::eq::EqGadget;
use ark_r1cs_std::alloc::AllocVar;
use ark_r1cs_std::{prelude::*, ToConstraintFieldGadget, select::CondSelectGadget};
use ark_r1cs_std::fields::fp::FpVar;
use ark_snark::SNARK;
// use ark_snark::SNARK::{VerificationKey, ProvingKey};
use ark_serialize::*;

fn print_type_of<T>(_: &T) {
    debug!("{}", std::any::type_name::<T>());
}

fn little_endian_get_bit(value:u8) -> Vec<u8>{
    let mut return_vec = Vec::new();
    // check all the bits from LSB to MSB
    for i in 0..8 {
        let mask = 1 << i;
        let bit_is_set = (mask & value) > 0;
        return_vec.push(bit_is_set as u8);
    }
    return_vec
}
fn big_endian_get_bit(value:u8) -> Vec<u8>{
    let mut return_vec = Vec::new();
    // check all the bits from LSB to MSB
    for i in 0..8 {
        let mask = 128 >> i;
        let bit_is_set = (mask & value) > 0;
        return_vec.push(bit_is_set as u8);
    }
    return_vec
}

/// Finalizes a SHA256 gadget and gets the bytes
// fn finalize_var(sha256_var: Sha256Gadget<Fr>) -> Vec<u8> {
//     sha256_var.finalize().unwrap().value().unwrap().to_vec()
// }
/// Finalizes a native SHA256 struct and gets the bytes
// fn finalize(sha256: Sha256) -> Vec<u8> {
//     sha256.finalize().to_vec()
// }

// Demo: DG1 of passport type TD3 preimage satisfy >= age_requirement
// NOTE: Currently support ICAO TD3 passport
const DG1_TD3_LEN: usize = 93;
struct SHA2PreimageCircuit {
    input: Vec<u8>,
    expected_hash: Vec<u8>,
    public_input_verification: Vec<ConstraintF>,
    client_nonce: Vec<u8>,
    age_requirement: u8,
    cur_year: u8
}
impl SHA2PreimageCircuit{
    pub fn new(input: Vec<u8>, expected_hash: Vec<u8>, client_nonce: Vec<u8>,age_req: u8, cur_year: u8) -> Self {
        let mut u8bits_vec:Vec<u8> = Vec::<u8>::new();
        for i in 0..expected_hash.len(){
            let mut temp =  little_endian_get_bit(expected_hash[i]);
            u8bits_vec.append(&mut temp);
        }


        let mut public_input_verification = Vec::new();
        for i in 0..u8bits_vec.len(){
            public_input_verification.push(ConstraintF::from(u8bits_vec[i]));
        }

        public_input_verification.push(Fr::from(cur_year));
        public_input_verification.push(Fr::from(age_req));

        debug!("Public Input VectorLength: {}",public_input_verification.len());

        SHA2PreimageCircuit {
            input: input,
            expected_hash: expected_hash,
            public_input_verification: public_input_verification,
            client_nonce: client_nonce,
            age_requirement: age_req,
            cur_year: cur_year
        }
    }

    pub fn new_for_keygen_only() -> Self{
        SHA2PreimageCircuit {
            input: vec![0; DG1_TD3_LEN],
            expected_hash: vec![0; 32],
            public_input_verification:  Vec::new(),
            client_nonce:  vec![0; 32],
            age_requirement: 0,
            cur_year: 0
        }
    }

    /// Witnesses bytes
    fn to_byte_vars(cs: impl Into<Namespace<ConstraintF>>, data: &[u8]) -> Vec<UInt8<ConstraintF>> {
        let cs = cs.into().cs();
        UInt8::new_witness_vec(cs, data).unwrap()
    }

    fn extract_to_publicinput(& self) -> Vec<ConstraintF>{
        //Need to pass in the correct Var in correct order exposed.
        return self.public_input_verification.clone();
    }
}

impl ConstraintSynthesizer<ConstraintF> for SHA2PreimageCircuit {
    fn generate_constraints(mut self, cs: ConstraintSystemRef<ConstraintF>) -> Result<(), SynthesisError> {
        debug!("Generating Constraints");
        // let unit = ();
        let unit_var = UnitVar::default();
        // self.input[0] = 1; // Try to not enforce_equal then change input to something else => Will pass the verification.
        let dg1_var =  UInt8::new_witness_vec(cs.clone(), &self.input)?; //Private witness
        // let asserted_digest_var =  UInt8::new_(cs.clone(), &self.input)?;\

        // This call SHA256::digest() which is a wrapper for going thorugh SHA2 then pack the result into a digestVar.
        debug!("Evaluating SHA256");
        let dg1_digest_var =  <Sha256Gadget<ConstraintF> as CRHSchemeGadget<Sha256, ConstraintF>>::evaluate(
            &unit_var,
            &dg1_var,
        )
        .unwrap();

        let mut client_nonce_var = UInt8::new_witness_vec(cs.clone(), &self.client_nonce).unwrap();
        let randomized_dg1 =  &mut dg1_digest_var.0.clone(); //Private witness
        randomized_dg1.append(&mut client_nonce_var);
        let randomized_digest_var = <Sha256Gadget<ConstraintF> as CRHSchemeGadget<Sha256, ConstraintF>>::evaluate(
            &unit_var,
            &randomized_dg1,
        )
        .unwrap();

        // self.expected_hash[0]=1;
        let expected_hash_var = DigestVar::new_input(cs.clone(), || Ok(self.expected_hash.clone())).unwrap(); //The closure || {} provide optional assignment (during prove but not during setup)
        debug!("Enforcing digest equal");
        randomized_digest_var.enforce_equal(&expected_hash_var).unwrap();

        // Prove age now
        // Byte pos 62-63 [0-indexed] is the year [TD3 specification]
        // See https://www.icao.int/Meetings/TAG-MRTD/TagMrtd22/TAG-MRTD-22_WP03-rev.pdf
        // Note that DG1 read from passport contains header (not part of the MRZ text). 
        // Header: Tag.Num 0x61 + 0x5b (remaining length of 91) + 0x5f1f (MRZ)
        // Stripping the header will give the position as in the doc.
        
        // int_from_ascii, ascii "0" = 48=0b110000, ascii "9"=57=0b111001
        //let ascii_digit_start = 48;
        //let digit_start_var = UInt8::new_constant(cs.clone(), ascii_digit_start).unwrap();

        //Drop the last two bits (little endian)
        debug!("Proving Age");
        let year_onesdigit_tmple = dg1_var[63].to_bits_le().unwrap();
        let year_tensdigit_tmple = dg1_var[62].to_bits_le().unwrap();
        let year_onesdigit_var = UInt8::from_bits_le(&year_onesdigit_tmple);
        let year_tensdigit_var = UInt8::from_bits_le(&year_tensdigit_tmple);
        let one_digit_fr_var = &vec![year_onesdigit_var].to_constraint_field().unwrap()[0];
        let ten_digit_fr_var = &vec![year_tensdigit_var].to_constraint_field().unwrap()[0];
        //ASCII to Int Repr
        let ten_mult_var = FpVar::<ConstraintF>::new_constant(cs.clone(), Fr::from(10)).unwrap();
        let ascii_digitoffset_var = FpVar::<ConstraintF>::new_constant(cs.clone(), Fr::from(48)).unwrap();
        let dob_year_fr_var = one_digit_fr_var.sub(&ascii_digitoffset_var).add(ten_digit_fr_var.sub(&ascii_digitoffset_var).mul(&ten_mult_var));

        // Wrap Cur_year ie if dobyear<=cur_year do substraction and add it
        let cur_year_var =  FpVar::<ConstraintF>::new_input(cs.clone(), || Ok(Fr::from(self.cur_year))).unwrap();
        let is_leq_var = dob_year_fr_var.is_cmp(&cur_year_var,Ordering::Less,true).unwrap();

        let dobyear_to_curyear = (&cur_year_var).sub(&dob_year_fr_var);
        let onehundered_var =  FpVar::<ConstraintF>::new_constant(cs.clone(), Fr::from(100)).unwrap();
        let dobyear_to_99_to_curyear = onehundered_var.sub(&dob_year_fr_var).add(&cur_year_var);

        let age_var = CondSelectGadget::conditionally_select(&is_leq_var, &dobyear_to_curyear, &dobyear_to_99_to_curyear).unwrap();

        let age_req_var = FpVar::<ConstraintF>::new_input(cs.clone(), || Ok(Fr::from(self.age_requirement))).unwrap();
        debug!("Age:{:?},",age_var.value());
        debug!("Greater than or equal to:{:?},",age_req_var.value());
        let is_equal_check = true;
        age_var.enforce_cmp(&age_req_var,Ordering::Greater,is_equal_check).unwrap();

        Ok(())
    }
}

// ProvingKey, VerifyingKey
pub fn generate_keys() -> (Vec<u8>, Vec<u8>){
    use rand::prelude::*;
    use rand_chacha::ChaCha20Rng;
    let mut rng = ChaCha20Rng::from_entropy();
    debug!("Generating Key");
    let hash_preimage_circuit = SHA2PreimageCircuit::new_for_keygen_only();
    debug!("New Circuit Instance");

    // circuit_specific_setup
    let start = Instant::now();
    let (pk, vk) = Groth16::<Bls12_381>::circuit_specific_setup(hash_preimage_circuit, &mut rng).unwrap();
    let duration = start.elapsed();
    debug!("Time elapsed in KeyGen() is: {:?}", & duration);

    let mut vk_buffer = Vec::new();
    (&vk).serialize_uncompressed(&mut vk_buffer).unwrap();

    let mut pk_buffer = Vec::new();
    (&pk).serialize_uncompressed(&mut pk_buffer).unwrap();
    debug!("Time elapsed in KeyGen+Serialization() is: {:?}", & duration);

    return (pk_buffer,vk_buffer)
}

pub fn parse_verf_key(verf_key_raw: Vec<u8>)  -> VerifyingKey<Bls12_381>{
    VerifyingKey::<Bls12_381>::deserialize_uncompressed_unchecked(&verf_key_raw[..]).unwrap()
}
pub fn parse_keys(prov_key_raw: Vec<u8>, verf_key_raw: Vec<u8>) -> (ProvingKey<Bls12_381>, VerifyingKey<Bls12_381>){
    //Unchecked if the key is validated already. Better performance.
    debug!("Parsing Key");
    let start = Instant::now();
    let pk= ProvingKey::<Bls12_381>::deserialize_uncompressed_unchecked(&prov_key_raw[..]).unwrap();
    let vk = VerifyingKey::<Bls12_381>::deserialize_uncompressed_unchecked(&verf_key_raw[..]).unwrap();
    let duration = start.elapsed();
    debug!("Time elapsed in {} is: {:?}", "Parse Key" , &duration);

    return (pk,vk);
}
pub fn parse_proof(proof_raw: Vec<u8>) -> Proof::<Bls12_381>{
    debug!("Parsing Proof");
    let start = Instant::now();
    let proof= Proof::<Bls12_381>::deserialize_uncompressed_unchecked(&proof_raw[..]).unwrap();
    let duration = start.elapsed();
    debug!("Time elapsed in {} is: {:?}", "Proof Prasing" , &duration);
    proof
}

pub fn fidoac_prove_main(prov_key_raw: Vec<u8>, verf_key_raw: Vec<u8>, dg1: Vec<u8>,client_nonce:Vec<u8>,age_gt: i32,cur_year:i32) -> (bool,Vec<u8>, Vec<u8>) {
    debug!("FidoAC-Prove");

    assert!(age_gt <= 255, "Since YY format is supported, < 100 year is supported");
    assert!(cur_year <= 100,"Only YY format is supported");
    assert!(dg1.len() == DG1_TD3_LEN, "DG1 length is not TD3 len =/=93"); //Only Support TD3 at the moment

    let cs = ConstraintSystem::<Fr>::new_ref();
    let unit = ();
    let unit_var = UnitVar::default();
    // const len: u32 = 32; //Input Length
    // let input_str = rand::thread_rng().gen::<[u8; len]>();
    const LEN: usize = 93; //Input Length
    let rng = ark_std::test_rng();
    let dg1_str = dg1.clone();
    // rng.fill_bytes(&mut input_str);
    let dg1_digest_var =  <Sha256Gadget<ConstraintF> as CRHSchemeGadget<Sha256, ConstraintF>>::evaluate(
        &unit_var,
        &SHA2PreimageCircuit::to_byte_vars(ns!(cs, "input"), &dg1_str),
    )
    .unwrap();
    let mut client_nonce_var = UInt8::new_witness_vec(cs.clone(), &client_nonce).unwrap();
    let randomized_dg1 =  &mut dg1_digest_var.0.clone(); //Private witness
    randomized_dg1.append(&mut client_nonce_var);
    let randomized_digest_var = <Sha256Gadget<ConstraintF> as CRHSchemeGadget<Sha256, ConstraintF>>::evaluate(
        &unit_var,
        &randomized_dg1,
    )
    .unwrap();

    let dg1_hash = <Sha256 as CRHScheme>::evaluate(&unit, dg1_str.clone()).unwrap();
    let mut randomized_dg1 = dg1_hash;
    randomized_dg1.append(&mut client_nonce.clone() );
    let expected_output = <Sha256 as CRHScheme>::evaluate(&unit, randomized_dg1.clone()).unwrap();
    debug!("expected_output:{:?}",expected_output);
    debug!("Randomzied_Digest_var:{:?}",randomized_digest_var.value().unwrap().to_vec());
    assert_eq!(
        randomized_digest_var.value().unwrap().to_vec(),
        expected_output.clone(),
        "CRH error at length {}",
        LEN
    );
    debug!("Asserting Preimage Complete");

    // let (pk_raw, vk_raw) = generate_keys(); //Debug use only
    // let (pk, vk)= parse_keys(pk_raw, vk_raw); // Debug use only
    let (pk, vk)= parse_keys(prov_key_raw, verf_key_raw); 
    debug!("Parsed Key Complete");

    let proof_vec = fidoac_prove::<Fr>(pk, vk.clone(),dg1_str.clone(),expected_output.clone(),client_nonce,age_gt as u8,cur_year as u8);
    let is_proof_valid = fidoac_verify::<Fr>(vk, parse_proof(proof_vec.clone()), expected_output.clone(), age_gt as u8, cur_year as u8);
    
    return (is_proof_valid, proof_vec, expected_output)
}

fn fidoac_prove<ConstraintF>(pk: ProvingKey<Bls12_381>, vk: VerifyingKey<Bls12_381>,input_vec: Vec<u8>,expected_hash_vec: Vec<u8>, client_nonce:Vec<u8>,age_gt: u8, cur_year: u8) -> (Vec<u8>) {
    use rand::prelude::*;
    use rand_chacha::ChaCha20Rng;
    // use rand::FromEntropy;
    let mut rng = ChaCha20Rng::from_entropy();
    // let rng = &mut ark_std::test_rng(); //replace the rng

    let hash_preimage_circuit = SHA2PreimageCircuit::new(input_vec.clone(),expected_hash_vec.clone(), client_nonce.clone(), age_gt,cur_year);
    let public_input = hash_preimage_circuit.extract_to_publicinput();
    debug!( "Public input Length final: {}", public_input.len() );

    //Need to run the generate constraints before this can be used.
    let hash_preimage_circuit = SHA2PreimageCircuit::new(input_vec.clone(),expected_hash_vec.clone(),client_nonce.clone(), age_gt,cur_year);
    debug!("Proving");
    let start = Instant::now();
    let proof = Groth16::<Bls12_381>::prove(&pk, hash_preimage_circuit, &mut rng).unwrap();
    let mut proof_buf = Vec::new();
    (&proof).serialize_uncompressed(&mut proof_buf).unwrap();
    let duration = start.elapsed();
    debug!("Time elapsed in Proving() is: {:?}", & duration);

    return proof_buf.clone()
}

pub fn fidoac_verify<ConstraintF>(vk: VerifyingKey<Bls12_381>, proof: Proof::<Bls12_381>, expected_hash_vec: Vec<u8>, age_gt: u8, cur_year: u8) -> bool {
    debug!("Verifying");
    let start = Instant::now();
    
    let pvk =  Groth16::<Bls12_381>::process_vk(&vk).unwrap();
    let empty:Vec<u8> = Vec::new();
    let hash_preimage_circuit = SHA2PreimageCircuit::new(empty.clone(),expected_hash_vec.clone(), empty.clone(), age_gt,cur_year);
    let public_input = hash_preimage_circuit.extract_to_publicinput();
    let is_valid_proof =  Groth16::<Bls12_381>::verify_with_processed_vk(&pvk, &public_input, &proof).unwrap();

    let duration = start.elapsed();
    debug!("Time elapsed in Verifying() is: {:?}", & duration);

    is_valid_proof
}

#[cfg(test)]
mod test {
    use super::*;
    
    #[test]
    fn my_test() {
        println!("Test Print");
    }
}