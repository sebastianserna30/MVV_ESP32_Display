from fastapi import FastAPI, HTTPException
from mvg import MvgApi
import uvicorn
from typing import List, Optional

app = FastAPI()

@app.get("/departures/{station_id}")
async def get_departures(
    station_id: str,
    transport_types: Optional[str] = None,
    limit: int = 10
):
    try:
        # Initialize MVG API with the station
        api = MvgApi(station_id)
        
        # Convert transport types from string to list if provided
        transport_types_list = None
        if transport_types:
            transport_types_list = transport_types.split(',')
        
        # Get departures
        departures = api.departures(limit=limit, transport_types=transport_types_list)
        return departures
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)