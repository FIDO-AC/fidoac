package anon.fidoac.service;

import android.util.Log;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.IOException;

import fi.iki.elonen.NanoHTTPD;

public class HttpServer extends NanoHTTPD {
    private Object data;
    private boolean isFetched;
    private String challenge;

    public HttpServer(Object data, String challenge) throws IOException {
        super(8080);
        this.data = data;
        this.challenge = challenge;
        start(NanoHTTPD.SOCKET_READ_TIMEOUT, true);
        System.out.println("\nRunning! Point your browsers to http://localhost:8080/ \n");
    }

    private void addCorsHeaders(Response resp){
        resp.addHeader("Access-Control-Allow-Origin", "*");
//            resp.addHeader("Access-Control-Allow-Origin", "https://fido.westeurope.cloudapp.azure.com");
        resp.addHeader("Access-Control-Max-Age", "3628800");
        resp.addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        resp.addHeader("Access-Control-Allow-Headers", "*");
    }

    @Override
    public Response serve(IHTTPSession session) {

        if(session.getMethod().equals(Method.OPTIONS)){
            Response resp= newFixedLengthResponse(Response.Status.OK, MIME_PLAINTEXT ,"success");
            addCorsHeaders(resp);
            return resp;
        }
        if(!session.getParameters().containsKey("challenge")){
            return newFixedLengthResponse(Response.Status.BAD_REQUEST, MIME_PLAINTEXT, "Missing challenge");
        }

        if(data != null && session.getParameters().get("challenge").get(0).equals(this.challenge)) {
            try {
                Log.i("FIDOAC Callback","Serving data");
                String dataJson = new ObjectMapper().writeValueAsString(data);
                Log.d("FIDOAC Callback", "Sending " + dataJson);
                isFetched = true;
                Response resp =  newFixedLengthResponse(Response.Status.OK, "application/json", dataJson);
                resp.addHeader("Cache-Control","no-cache, no-store, must-revalidate");
                addCorsHeaders(resp);
                return resp;
            } catch (JsonProcessingException e) {
                return newFixedLengthResponse(Response.Status.INTERNAL_ERROR, MIME_PLAINTEXT, "");
            }
        }else{
            return newFixedLengthResponse(Response.Status.BAD_REQUEST, MIME_PLAINTEXT, "");
        }
    }

    public boolean isDataFetched(){
        return isFetched;
    }



}
