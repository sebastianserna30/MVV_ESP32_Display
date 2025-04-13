// Configuration for MVG departures display
// Transport types: UBAHN, SBAHN, TRAM, BUS
// For multiple types, separate with comma, e.g., "TRAM,BUS"
// time_offset is in minutes, use "0" for immediate departures

static const Config configs[] = {
    {
        /* pretty_name           */ String("Stiglmaierpl."), // Display name for the station
        /* bahnhof             */ String("de:09162:170"),    // Stiglmaierplatz
        /* include_type        */ String("TRAM,UBAHN"),      // Include both tram and subway
        /* time_offset         */ String("0"),
    },
    // You can add more stations by copying this template:
    // {
    //     /* bahnhof             */ String("de:09162:430"), // Leuchtenbergring
    //     /* include_type        */ String("SBAHN"),        // S-Bahn only
    //     /* time_offset        */ String("0"),
    // },
};