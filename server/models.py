from pydantic import BaseModel
from typing import List, Dict, Any

class Entry(BaseModel):
    table: str
    data: Dict[str, Any]

class SyncRequest(BaseModel):
    client_id: str
    last_server_version: int
    entries: List[Entry]

class SyncResponse(BaseModel):
    new_server_version: int
    entries: List[Entry]