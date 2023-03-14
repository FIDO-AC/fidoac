import {AuthenticationCredentialJSON} from "@simplewebauthn/typescript-types";


export interface FidoACAuthenticationExtensionsClientOutputs extends AuthenticationExtensionsClientOutputs{
    fidoac?: any;
}

export interface FidoACAuthenticationCredentialJSON extends AuthenticationCredentialJSON{
    clientExtensionResults: FidoACAuthenticationExtensionsClientOutputs;
}
