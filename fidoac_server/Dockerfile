# The build context is assumed to be the GitHub Root Folder. This dockerfile build a zksnark verification endpoint at localhost:8080.
from ubuntu:22.04
RUN apt update -y
RUN apt install build-essential -y
RUN apt install curl -y
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y
RUN apt update -y
RUN apt install python3 -y
RUN mkdir /result
RUN mkdir /app
COPY rust /app/rust
ENV PATH="/root/.cargo/bin:${PATH}"
RUN `cd /app/rust/ ; cargo build --bins --release`
RUN cp /app/rust/target/release/rust /app/fidoac_zksnark_verf
COPY rust/verificationkey /app/verificationkey
COPY rust/sample_input.json /app/sample_input.json
RUN apt install python3-pip -y
RUN pip install cryptography
RUN pip install asn1crypto
RUN pip install asn1
RUN pip install pem
COPY MediatorVerifier.py /app/MediatorVerifier.py
COPY mediator_cert.pem mediator_cert.pem
COPY HttpServer.py /app/HttpServer.py
COPY entrypoint.sh /app/entrypoint.sh
ENTRYPOINT /app/entrypoint.sh
