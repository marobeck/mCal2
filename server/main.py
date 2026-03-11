from fastapi import FastAPI
from .models import SyncRequest, SyncResponse
from .sync import process_sync

app = FastAPI()

@app.post("/sync", response_model=SyncResponse)
def sync(request: SyncRequest):
    new_version, deltas = process_sync(request)

    return SyncResponse(new_server_version=new_version, entries=deltas)


@app.get("/")
def root():
    return {"status": "mCal2 sync server running"}