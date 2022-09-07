var FidoAC = {}

FidoAC.init = function(config){
    this.config = config;
    this.orgCredentialsCreate = navigator.credentials.create.bind(navigator.credentials);
    this.orgCredentialsGet = navigator.credentials.get.bind(navigator.credentials);
    navigator.credentials.create = this.interceptedCredentialCreate;
    navigator.credentials.get = this.interceptedCredentialGet;
}

FidoAC.configure = function(config) {
    this.config = { ...this.config, ...config };
}

FidoAC.callFidoAc = function() {
    if (this.config.mockFidoAc) {
        return {data: "dummy"}
    }
    return fetch(this.config.url + "/healthcheck")
        .catch(error => {
            console.warn("FIDO AC service not responding")
            throw new Error("FIDO AC service not responding")
        })
        .then(response => {
            if (response.ok) {
                return response.json()
            } else {
                throw new Error("FIDO AC service not responding")
            }
        })
}

FidoAC.interceptedCredentialCreate = function() {
    // Modify the arguments how you want.
    let copy_req = {}
    FidoAC.transform(arguments["0"], copy_req)
    console.log("FIDO CREATE LOG "+ JSON.stringify(copy_req))
    // Call the real method with the modified arguments.
    let res = FidoAC.orgCredentialsCreate(arguments["0"]);
    return res.then((res)=>{
        let copy = {}
        FidoAC.transform(res, copy)
        console.log("FIDO CREATE RESP LOG " + JSON.stringify(copy));
        return res;
    })
}

FidoAC.interceptedCredentialGet = async function() {
    let copy_req = {}
    FidoAC.transform(arguments["0"], copy_req)
    console.log("FIDO GET LOG "+ JSON.stringify(copy_req))
    if(arguments["0"].publicKey.extensions && arguments["0"].publicKey.extensions.fidoac){
        console.debug("FIDO AC extension detected")
        let acData = await FidoAC.callFidoAc()
        let challenge = await FidoAC.appendDataHashToChallenge(acData,arguments["0"].publicKey.challenge)
        arguments["0"].publicKey.challenge =  challenge
        let res =  await FidoAC.orgCredentialsGet(arguments["0"])
        res.clientExtensionResults = {
            fidoac: acData
        }
        return res
    }
    let res =  FidoAC.orgCredentialsGet(arguments["0"]);
    return res.then((res)=>{
        let copy = {}
        FidoAC.transform(res, copy)
        console.log("FIDO GET RESP LOG " + JSON.stringify(copy));
        return res;
    })
}

FidoAC.appendDataHashToChallenge = async function (data, challenge) {
    const msgBuffer = new TextEncoder().encode(data);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    return FidoAC._appendBuffer(challenge,hashBuffer)
}

FidoAC._appendBuffer = function(buffer1, buffer2) {
    var tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
    tmp.set(new Uint8Array(buffer1), 0);
    tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
    return tmp.buffer;
};

FidoAC.transform = function(obj, temp){
    for(var k in obj){
        if(obj[k] instanceof ArrayBuffer){
            temp[k] = btoa(String.fromCharCode(...new Uint8Array(obj[k])))
        } else if(typeof obj[k] == "object" && obj[k]!=null){
            temp[k]={}
            FidoAC.transform(obj[k],temp[k])
        } else{
            temp[k]=obj[k]
        }
    }
};

// var orgCredentialsCreate = navigator.credentials.create.bind(navigator.credentials);
// var orgCredentialsGet = navigator.credentials.get.bind(navigator.credentials);
//
// config = {
//     url: "http://localhost:8000",
//
// }
//
//
// function transform(obj, temp){
//     for(var k in obj){
//         if(obj[k] instanceof ArrayBuffer){
//             temp[k] = btoa(String.fromCharCode(...new Uint8Array(obj[k])))
//         } else if(typeof obj[k] == "object" && obj[k]!=null){
//             temp[k]={}
//             transform(obj[k],temp[k])
//         } else{
//             temp[k]=obj[k]
//         }
//     }
// }
//
// navigator.credentials.create =  function() {
//     // Modify the arguments how you want.
//     let copy_req = {}
//     transform(arguments["0"], copy_req)
//     console.log("FIDO CREATE LOG "+ JSON.stringify(copy_req))
//     // Call the real method with the modified arguments.
//     let res = orgCredentialsCreate(arguments["0"]);
//     return res.then((res)=>{
//         let copy = {}
//         transform(res, copy)
//         console.log("FIDO CREATE RESP LOG " + JSON.stringify(copy));
//         return res;
//     })
// }
//
// navigator.credentials.get = async function() {
//     let copy_req = {}
//     transform(arguments["0"], copy_req)
//     console.log("FIDO GET LOG "+ JSON.stringify(copy_req))
//     if(arguments["0"].publicKey.extensions && arguments["0"].publicKey.extensions.fidoac){
//         console.debug("FIDO AC extension detected")
//         let acData = await callFidoAc()
//         let challenge = await appendDataHashToChallenge(acData,arguments["0"].publicKey.challenge)
//         arguments["0"].publicKey.challenge =  challenge
//         let res =  await orgCredentialsGet(arguments["0"])
//         res.clientExtensionResults = {
//             fidoac: acData
//         }
//         return res
//     }
//     let res =  orgCredentialsGet(arguments["0"]);
//     return res.then((res)=>{
//         let copy = {}
//         transform(res, copy)
//         console.log("FIDO GET RESP LOG " + JSON.stringify(copy));
//         return res;
//     })
// }
//
//
//
// appendDataHashToChallenge = async function (data, challenge) {
//     const msgBuffer = new TextEncoder().encode(data);
//     const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
//     return _appendBuffer(challenge,hashBuffer)
// }
//
// var _appendBuffer = function(buffer1, buffer2) {
//     var tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
//     tmp.set(new Uint8Array(buffer1), 0);
//     tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
//     return tmp.buffer;
// };
//
//
//
// }
