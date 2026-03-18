```
              ______      _____ 
   ____ ___  / ____/___ _/ /__ \
  / __ `__ \/ /   / __ `/ /__/ /
 / / / / / / /___/ /_/ / // __/ 
/_/ /_/ /_/\____/\__,_/_//____/ 
```

A multiplatform calendar app based on the concept of taskblocks, ideal for organizing projects and habits.

# Work In Progress!

This project is heavily work in progress and is subject to change over time as features are gradually completed. Currently the app supports:

- Adding and managing tasks and habits within and between taskblocks
- Weekday and frequency based habit goals for habit tracking.
- A basic ordering system based on priority and time remaining.
    - Highest urgency task block is on the left, highest urgency task is on the top
- Task block statuses for marking projects as completed, paused, or pinned.
- Syncronizing data between cilents

# Objectives

As I accumulate projects, I found that my tasks no longer were independent of each other, they were all for some larger goal. However most project management software is designed for very long term projects with a strict end product. Taskblocks warps the concept of a project into any broad catagory, a task block, including concrete projects such as a game, to indefinite groups such as health. To account for this habit tracking is built directly into each task block.

Design objectives:
- A todo list that can be quickly added to. 
- Clear work-life seperation between tasks, habits, and hobbies
- A protocol for bringing up any task, choosing the most pressing or convient task.
- Time blocking, time groups should be given parameters for time blocking, allowing for the user to allocate time for specific types of tasks.

# mCal2 Secure Sync Setup Guide

This guide explains how to securely set up the **mCal2 synchronization system** using:

* SQLite (client databases)
* Python server (FastAPI + Uvicorn)
* Mutual TLS (mTLS) authentication

---

# Architecture Overview

```
Client (Qt app)
    ⇅ HTTPS (mTLS)
Server (Python + SQLite)
    ⇅
Other Clients
```

Each client:

* Tracks local changes
* Pushes updates to the server
* Pulls updates from the server

---

# Host Machine Setup (Server)


## 1. Install Dependencies

Upload contents of server/ to host machine.

```bash
python3 -m venv .env
source .env/bin/activate
pip install -r requirements.txt
```

---

## 2. Generate Certificates

### Create Certificate Authority (CA)

```bash
openssl req -x509 -newkey rsa:4096 \
  -keyout ca.key \
  -out ca.crt \
  -days 365 \
  -nodes \
  -subj "/CN=mcal-ca"
```

---

### Create Server Certificate

```bash
openssl req -newkey rsa:2048 -nodes \
  -keyout server.key \
  -out server.csr \
  -subj "/CN=sync-server" \
  -addext "subjectAltName=DNS:sync-server,IP:127.0.0.1"
```

Sign it:

```bash
openssl x509 -req \
  -in server.csr \
  -CA ca.crt \
  -CAkey ca.key \
  -CAcreateserial \
  -out server.crt \
  -days 365 \
  -copy_extensions copy
```

---

## 3. Create Server Database

Set environment variable:

```bash
export MCAL_SERVER_DB=/absolute/path/to/server.db
```

---

## 4. Start Server (with TLS + client auth)

```bash
uvicorn server.main:app \
  --host 0.0.0.0 \
  --port 8443 \
  --ssl-keyfile server.key \
  --ssl-certfile server.crt \
  --ssl-ca-certs ca.crt \
  --ssl-cert-reqs 2
```

### Notes

* `--ssl-cert-reqs 2` enforces **client certificate authentication**
* Only clients signed by your CA can connect

---

## 5. Firewall / Network

If using a VPS or VPN:

* Open port `8443`
* Ensure hostname matches certificate (e.g. `sync-server`)

---

# Client Setup

Repeat this section for **each device**.

---

## 1. Generate Client Certificate

```bash
openssl req -newkey rsa:2048 -nodes \
  -keyout client.key \
  -out client.csr \
  -subj "/CN=client1"
```

Sign with CA:

```bash
openssl x509 -req \
  -in client.csr \
  -CA ca.crt \
  -CAkey ca.key \
  -CAcreateserial \
  -out client.crt \
  -days 365
```

---

## 2. Directory Structure

```
client/
├── mcal2
├── assets/
│   └── settings.ini
└── certs/
    ├── client.crt
    ├── client.key
    └── ca.crt
```

---

## 3. Configure `settings.ini`

**Important! Client is configured to not syncronize by default**

```ini
[Sync]
enabled=true

server_address=httpss://sync-server:8443/sync
server_port=8443

client_id=client1

client_cert_path=certs/client.crt
client_key_path=certs/client.key
server_ca_path=certs/ca.crt
```

---

## 4. Important Notes

### Hostname MUST match certificate

If your server cert uses:

```
CN=sync-server
```

Then your client must connect to:

```
https://sync-server:8443
```

NOT:

```
https://127.0.0.1:8443
```

Unless your cert includes that IP in SAN.

---

## 5. Run Client

```bash
./mcal2
```

If configured correctly:

* Client connects securely
* Sync button should be visible in bottom right corner