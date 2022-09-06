var orgCredentialsCreate = navigator.credentials.create.bind(navigator.credentials);
var orgCredentialsGet = navigator.credentials.get.bind(navigator.credentials);

config = {
    url: "http://localhost:8000",

}


function transform(obj, temp){
    for(var k in obj){
        if(obj[k] instanceof ArrayBuffer){
            temp[k] = btoa(String.fromCharCode(...new Uint8Array(obj[k])))
        } else if(typeof obj[k] == "object" && obj[k]!=null){
            temp[k]={}
            transform(obj[k],temp[k])
        } else{
            temp[k]=obj[k]
        }
    }
}

navigator.credentials.create =  function() {
    // Modify the arguments how you want.
    let copy_req = {}
    transform(arguments["0"], copy_req)
    console.log("FIDO CREATE LOG "+ JSON.stringify(copy_req))
    // Call the real method with the modified arguments.
    let res = orgCredentialsCreate(arguments["0"]);
    return res.then((res)=>{
        let copy = {}
        transform(res, copy)
        console.log("FIDO CREATE RESP LOG " + JSON.stringify(copy));
        return res;
    })
}

navigator.credentials.get = async function() {
    let copy_req = {}
    transform(arguments["0"], copy_req)
    console.log("FIDO GET LOG "+ JSON.stringify(copy_req))
    if(arguments["0"].publicKey.extensions && arguments["0"].publicKey.extensions.fidoac){
        console.debug("FIDO AC extension detected")
        let acData = await callFidoAc()
        let challenge = await appendDataHashToChallenge(acData,arguments["0"].publicKey.challenge)
        arguments["0"].publicKey.challenge =  challenge
        let res =  await orgCredentialsGet(arguments["0"])
        res.clientExtensionResults = {
            fidoac: acData
        }
        return res
        // arguments["0"].publicKey.challenge =  challenge
        //     .then((data) => acData = data)
        //     .then(()=> appendDataHashToChallenge(acData,arguments["0"].publicKey.challenge))
        //     .then((challenge) => arguments["0"].publicKey.challenge =  challenge)
        //     .then(() =>  orgCredentialsGet(arguments["0"]))
        //     .then( (res) => {
        //         res.clientExtensionResults.fidoac = acData
        //         return res
        //     })
    }
    debugger
    let res =  orgCredentialsGet(arguments["0"]);
    return res.then((res)=>{
        let copy = {}
        transform(res, copy)
        console.log("FIDO GET RESP LOG " + JSON.stringify(copy));
        return res;
    })

}



appendDataHashToChallenge = async function (data, challenge) {
    const msgBuffer = new TextEncoder().encode(data);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    return _appendBuffer(challenge,hashBuffer)
}

var _appendBuffer = function(buffer1, buffer2) {
    var tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
    tmp.set(new Uint8Array(buffer1), 0);
    tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
    return tmp.buffer;
};

callFidoAc = function() {
    return fetch(config.url+"/healthcheck")
        .catch(error => {
            console.warn("FIDO AC service not responding")
            throw new Error("FIDO AC service not responding")
        })
        .then(response => {
            if (response.ok) {
                return response.json()
            } else{
                return {data:"dummy"}
                // throw new Error("FIDO AC service not responding")
            }
            // TODO call service

        })

}
