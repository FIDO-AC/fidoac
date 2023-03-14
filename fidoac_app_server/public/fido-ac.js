var FidoAC = {}

FidoAC.init = function(config){
    this.defaultConfig = {
        appid: "anon.fidoac"
    }
    this.config = {...this.defaultConfig, ...config};
    this.orgCredentialsCreate = navigator.credentials.create.bind(navigator.credentials);
    this.orgCredentialsGet = navigator.credentials.get.bind(navigator.credentials);
    navigator.credentials.create = this.interceptedCredentialCreate;
    navigator.credentials.get = this.interceptedCredentialGet;
}

FidoAC.configure = function(config) {
    this.config = { ...this.config, ...config };
}

FidoAC.getDeepLink = function (appid, challenge, origin, ageQueryGT){
    return `android-app://${appid}/https/fidoacsource.z13.web.core.windows.net/?challenge=${challenge}&origin=${origin}&ageQueryGT=${ageQueryGT}#Intent;action=${appid}.START_SERVICE;end`
}

FidoAC.callFidoAc = function(challenge) {

    if (this.config.mockFidoAc) {
        return new Promise(resolve => setTimeout(function () {resolve({data: "dummy"})}, 6000))
    }
    return new Promise(function(resolve, reject) {
        var i = 0
        function getACData(){
            fetch(FidoAC.config.url + "/?challenge="+challenge)
            .then(response => {
                if (response && response.ok) {
                    resolve(response.json())
                    return
                } else {
                    console.warn("FIDO AC service not responding")
                }
            }).catch(error => {
                    console.warn("FIDO AC service not responding")
                })
            i = i + 1
            if(i >20){
                reject("Max number of requests reached")
                return
            }
            setTimeout(getACData, 5000);
        }
        setTimeout(getACData, 5000);
    })

}

FidoAC.getModalBackground = function (){
    var background = document.createElement('div')
    background.style.position = "fixed"
    background.style.top = "0"
    background.style.right = "0"
    background.style.left = "0"
    background.style.bottom = "0"
    background.style.zIndex = "99999";
    background.style.width = "100%"
    background.style.height = "100%"
    background.style.backgroundColor = "#00000082"
    return background
}

FidoAC.getModalDiv = function (){
    var dev = document.createElement('div')
    dev.style.position = "fixed"
    dev.style.top = "0"
    dev.style.right = "0"
    dev.style.left = "0"
    dev.style.bottom = "0"
    dev.style.zIndex = "99999";
    dev.style.width = "50%"
    dev.style.minWidth = "300px"
    dev.style.height = "50%"
    dev.style.backgroundColor = "white"
    dev.style.margin = "auto"
    dev.style.display = "flex"
    dev.style.flexDirection = "column"
    dev.style.alignItems = "center"
    dev.style.justifyContent = "center"
    return dev;
}

FidoAC.createSelectModal = function(decisionCallback){
    var background = FidoAC.getModalBackground()
    var dev = FidoAC.getModalDiv()

    var buttonFidoAC = document.createElement('button')
    buttonFidoAC.innerHTML = "Use FIDO-AC"
    buttonFidoAC.type = "button";
    buttonFidoAC.style.marginBottom = "10px"
    buttonFidoAC.onclick = function() { decisionCallback("fido-ac",background) }

    var buttonRegularFido = document.createElement('button')
    buttonRegularFido.innerHTML = "Use regular FIDO"
    buttonRegularFido.type = "button";
    buttonRegularFido.onclick = function() { decisionCallback("regular-fido",background) }

    dev.appendChild(buttonFidoAC)
    dev.appendChild(buttonRegularFido)
    background.appendChild(dev)

    // var iframe = document.createElement('iframe');
    // var html = '<a href="android-app://com.example.fidoac">Continue with FIDOAC</a>';
    // iframe.src = 'data:text/html;charset=utf-8,' + encodeURI(html);
    document.body.appendChild(background);
}

FidoAC.createOpenAppModal = function(challenge){
    var background = FidoAC.getModalBackground()
    var dev = FidoAC.getModalDiv()

    var applink = document.createElement('a')
    // applink.href="android-app://com.example.fidoac/http/example.com/?challenge="+challenge+"#Intent;action=com.example.fidoac.START_SERVICE;end"
    applink.href=FidoAC.getDeepLink(this.config.appid,challenge,location.origin,20)
    document.getElementById("authDebug").innerHTML += challenge
    applink.innerHTML="Open FIDO AC App"
    applink.onclick = function () { background.remove() }
    applink.style.appearance = "button"
    applink.style["-moz-appearance"] = "button"
    applink.style["-webkit-appearance"] = "button"
    applink.style.textDecoration = "none"
    applink.style.color = "black"
    applink.style.padding = "15px 30px"
    applink.style.border = "1px solid black"
    applink.style.cursor = "pointer"

    dev.appendChild(applink)
    background.appendChild(dev)
    document.body.appendChild(background);
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

FidoAC.getUserDecision = async function() {
    return new Promise(function(resolve, reject) {
        function decisionCallback(decision, modal) {
            modal.remove()
            resolve(decision)
        }
        FidoAC.createSelectModal(decisionCallback)
    })
}

FidoAC.handleFidoACExtension = async function(arguments) {
    let challenge = arguments["0"].publicKey.challenge
    let challengeBase64 = btoa(String.fromCharCode.apply(null, new Uint8Array(challenge))).replaceAll("+","-").replaceAll("/","_")
    console.debug("User selected FIDO AC")
    let acDataPromise =  FidoAC.callFidoAc(challengeBase64)
    FidoAC.createOpenAppModal(challengeBase64)
    let acData = await acDataPromise;
    challenge = await FidoAC.appendDataHashToChallenge(acData,challenge)
    arguments["0"].publicKey.challenge =  challenge
    let res =  await FidoAC.orgCredentialsGet(arguments["0"])
    res.clientExtensionResults = {
        fidoac: acData
    }
    document.getElementById("authDebug").innerHTML += res.clientExtensionResults
    return res
}

FidoAC.interceptedCredentialGet = async function() {
    let copy_req = {}
    FidoAC.transform(arguments["0"], copy_req)
    console.log("FIDO GET LOG "+ JSON.stringify(copy_req))
    if(arguments["0"].publicKey.extensions && arguments["0"].publicKey.extensions.fidoac){
        console.debug("FIDO AC extension detected")
        let decision = await FidoAC.getUserDecision();
        if(decision === "fido-ac") {

            return FidoAC.handleFidoACExtension(arguments)
        }
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
