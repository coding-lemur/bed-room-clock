double round2(double value)
{
    return (int)(value * 100 + 0.5) / 100.0;
}

String getDeviceId(String hostname)
{
    int index = hostname.lastIndexOf('-');
    int length = hostname.length();
    return hostname.substring(index + 1, length);
}

int getRssiAsQuality(int rssi)
{
    int quality = 0;

    if (rssi <= -100)
    {
        quality = 0;
    }
    else if (rssi >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (rssi + 100);
    }

    return quality;
}

unsigned long getUnixTime()
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return 0;
    }

    time(&now);
    return now; // unix time
}

void loadFilesToArray(File root, JsonArray list)
{
    File file = root.openNextFile();

    while (file)
    {
        list.add(file.name());
        file = root.openNextFile();
    }
}