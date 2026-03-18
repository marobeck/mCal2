import subprocess
import time
import os
import shutil

TMP = "tmp"
CERTS = os.path.join(TMP, "certs")

CLIENT1 = os.path.join(TMP, "client1")
CLIENT2 = os.path.join(TMP, "client2")

SERVER_PORT = "8443"


def run(cmd):
    subprocess.run(cmd, check=True)


def generate_certs():
    os.makedirs(CERTS, exist_ok=True)

    # ----- CA -----
    run(
        [
            "openssl",
            "req",
            "-x509",
            "-newkey",
            "rsa:4096",
            "-keyout",
            f"{CERTS}/ca.key",
            "-out",
            f"{CERTS}/ca.crt",
            "-days",
            "365",
            "-nodes",
            "-subj",
            "/CN=mcal-test-ca",
        ]
    )

    # ----- SERVER -----
    run(
        [
            "openssl",
            "req",
            "-newkey",
            "rsa:2048",
            "-nodes",
            "-keyout",
            f"{CERTS}/server.key",
            "-out",
            f"{CERTS}/server.csr",
            "-subj",
            "/CN=localhost",
        ]
    )

    run(
        [
            "openssl",
            "x509",
            "-req",
            "-in",
            f"{CERTS}/server.csr",
            "-CA",
            f"{CERTS}/ca.crt",
            "-CAkey",
            f"{CERTS}/ca.key",
            "-CAcreateserial",
            "-out",
            f"{CERTS}/server.crt",
            "-days",
            "365",
        ]
    )

    # ----- CLIENT 1 -----
    run(
        [
            "openssl",
            "req",
            "-newkey",
            "rsa:2048",
            "-nodes",
            "-keyout",
            f"{CERTS}/client1.key",
            "-out",
            f"{CERTS}/client1.csr",
            "-subj",
            "/CN=client1",
        ]
    )

    run(
        [
            "openssl",
            "x509",
            "-req",
            "-in",
            f"{CERTS}/client1.csr",
            "-CA",
            f"{CERTS}/ca.crt",
            "-CAkey",
            f"{CERTS}/ca.key",
            "-CAcreateserial",
            "-out",
            f"{CERTS}/client1.crt",
            "-days",
            "365",
        ]
    )

    # ----- CLIENT 2 -----
    run(
        [
            "openssl",
            "req",
            "-newkey",
            "rsa:2048",
            "-nodes",
            "-keyout",
            f"{CERTS}/client2.key",
            "-out",
            f"{CERTS}/client2.csr",
            "-subj",
            "/CN=client2",
        ]
    )

    run(
        [
            "openssl",
            "x509",
            "-req",
            "-in",
            f"{CERTS}/client2.csr",
            "-CA",
            f"{CERTS}/ca.crt",
            "-CAkey",
            f"{CERTS}/ca.key",
            "-CAcreateserial",
            "-out",
            f"{CERTS}/client2.crt",
            "-days",
            "365",
        ]
    )


def write_client_config(client_dir, name):
    settings = f"""
    [sync]
    enabled=true
    server_address=https://localhost:8443/sync
    server_port=8443

    client_id={name}

    client_cert_path={os.path.abspath(f"{CERTS}/{name}.crt")}
    client_key_path={os.path.abspath(f"{CERTS}/{name}.key")}
    server_ca_path={os.path.abspath(f"{CERTS}/ca.crt")}
    """


def main():

    os.makedirs(TMP, exist_ok=True)

    generate_certs()

    os.makedirs(CLIENT1, exist_ok=True)
    os.makedirs(CLIENT2, exist_ok=True)

    shutil.copy("../../../build/client/mcal2", CLIENT1)
    shutil.copytree(
        "../../../assets", os.path.join(CLIENT1, "assets"), dirs_exist_ok=True
    )

    shutil.copy("../../../build/client/mcal2", CLIENT2)
    shutil.copytree(
        "../../../assets", os.path.join(CLIENT2, "assets"), dirs_exist_ok=True
    )

    write_client_config(os.path.join(CLIENT1, "assets"), "client1")

    write_client_config(os.path.join(CLIENT2, "assets"), "client2")

    # server db
    server_db = os.path.abspath(os.path.join(TMP, "server.db"))
    os.environ["MCAL_SERVER_DB"] = server_db

    # start TLS server
    certs_abs = os.path.abspath(CERTS)

    server = subprocess.Popen(
        [
            "python3",
            "-m",
            "uvicorn",
            "server.main:app",
            "--host",
            "127.0.0.1",
            "--port",
            SERVER_PORT,
            "--ssl-keyfile",
            f"{certs_abs}/server.key",
            "--ssl-certfile",
            f"{certs_abs}/server.crt",
            "--ssl-ca-certs",
            f"{certs_abs}/ca.crt",
            "--ssl-cert-reqs",
            "2",
        ],
        cwd="../../../",
    )

    time.sleep(2)

    client1 = subprocess.Popen(["./mcal2"], cwd=CLIENT1)
    client2 = subprocess.Popen(["./mcal2"], cwd=CLIENT2)

    client1.wait()
    client2.wait()

    server.terminate()

    shutil.rmtree(TMP)


if __name__ == "__main__":
    main()
