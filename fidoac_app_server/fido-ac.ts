import base64url from "base64url";
import crypto from "crypto";
import fetch from 'node-fetch';

export class FidoAc {

    fidoacServerUrl: string;
    verifyEndpoint: string = "/fidoac-server/verify";

    constructor(fidoacServerUrl: string) {
        this.fidoacServerUrl = fidoacServerUrl;

    }

    appendToChallenge(currentChallenge: string, extensions: any) {
        if (extensions.fidoac) {
            const challengeBuffer = base64url.toBuffer(currentChallenge)
            const msgBuffer = new TextEncoder().encode(extensions.fidoac);
            const hashBuffer = crypto.createHash('sha256').update(msgBuffer).digest();
            return base64url(<Buffer>this._appendBuffer(challengeBuffer, hashBuffer))
        } else {
            return currentChallenge;
        }
    }

    verifyZKP(fidoAcData: any): Promise<boolean> {
        console.log(`FIDOAC ZKP verify request data ${fidoAcData}`)
        return fetch(this.fidoacServerUrl + this.verifyEndpoint, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(fidoAcData)
        })
            .then(res => res.json())
            .then(res => {
                console.log(`FIDOAC ZKP verify response ${res}`)
                return res.verified;
            })
            .catch(e => {
                console.log(e)
                return false;
            })
    }

    _appendBuffer(buffer1: Buffer, buffer2: Buffer) {
        var tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
        tmp.set(new Uint8Array(buffer1), 0);
        tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
        return tmp.buffer;
    }

}
