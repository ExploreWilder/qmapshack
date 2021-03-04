# Data Sources

A few global DEMs are available free of charge:

## Jonathan de Ferranti

The DEM is mainly based on SRTM 3" with a custom voidfill algorithm and the combination of alternative sources for the best global coverage including the highest and lowest latitudes. Alternative sources include Aster GDEM v2, Russian 200k and 100k, Nepal 50k, local 50k and 100k topos, 200m DEM from NSIDC.

**Good point**: the dataset is not just data processed by a computer, but carefully made with details in mind.

**Bad point**: the dataset does not include the latest sources available such as Aster GDEM v3.

* File Format: HGT
* Datum: WGS84/EGM96

More information: http://viewfinderpanoramas.org/dem3.html

## SRTMGL1 v3

The world famous dataset provided by the LP DAAC located at the U.S. DoI, USGS EROS Center in Sioux Falls. The German and Italian space agencies also collaborated in the SRTM data sets to generate a DEM of the Earth using radar interferometry.

**Good point**: because it is a famous data source, there are plenty of tools available.

**Bad point**: the dataset is limited between 60° N and 56° S latitude, and I found a few artifacts invisible in the Jonathan de Ferranti's dataset.

* File Format: HGT or NetCDF4
* Datum: WGS84/EGM96

More information: https://lpdaac.usgs.gov/products/srtmgl1v003/

## ASTGTM v3

The ASTGTM dataset is a collaborative effort between NASA and Japan's METI. Released in 2019, it is the latest publicly available dataset. Voids are filled by interpolation with Aster GDEM, PRISM, SRTM. The dataset includes a layer indicating the actual source of the data.

**Good point**: the coverage is great (83° N to 83° S) and - based on a few samples - artifacts visible in the SRTM v3 are invisible in ASTGTM v3.

**Bad point**: there are officially known issues like voids in Greenland, known inaccuracies and artifacts.

* File Format: GeoTIFF
* Datum: WGS84/EGM96

More information: https://lpdaac.usgs.gov/products/astgtmv003/

# Data Processing

The datasets listed above are very similar in the file format and use the same datum. An automated conversion with the GDAL toolkit would result in the same input data (tiles) to figure out the elevation at a specific location. Therefore, the algorithm is the same whatever the selected dataset.

An example of algorithm is the Inverse Distance Weighted.

# Acronyms

* **DEM**: Digital Elevation Model
* **GDEM**: Global DEM
* **ASTGTM**: Aster GDEM
* **SRTM**: Shuttle Radar Topography Mission
* **NSIDC**: National Snow and Ice Data Center
* **LP DAAC**: Land Processes Distributed Active Archive Center
* **DoI**: Department of the Interior
* **USGS**: U.S. Geological Survey
* **EROS**: Earth Resources Observation and Science
* **NASA**: National Aeronautics and Space Administration
* **METI**: Ministry of Economy, Trade, and Industry
* **GeoTIFF**: Georeferenced Tagged Image File Format
* **GDAL**: Geospatial Data Abstraction Library, by OSGeo
* **OSGeo**: Open Source Geospatial Foundation

