# The build context is assumed to be the GitHub Root Folder. This dockerfile build a zksnark verification endpoint at localhost:8080.
from ubuntu:22.04
RUN apt update -y
RUN apt install build-essential -y
RUN apt install curl -y
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y
RUN mkdir /result
RUN mkdir /app
COPY rust /app/rust
ENV PATH="/root/.cargo/bin:${PATH}"
RUN `cd /app/rust/ ; cargo build --bins --release`
RUN cp /app/rust/target/release/rust /app/fidoac_zksnark_verf
COPY rust/verificationkey /app/verificationkey
COPY rust/sample_input.json /app/sample_input.json
RUN apt update -y
RUN apt install python3 -y
COPY HttpServer.py /app/HttpServer.py
COPY entrypoint.sh /app/entrypoint.sh
ENTRYPOINT /app/entrypoint.sh
