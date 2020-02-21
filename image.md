# TiledC64 map data from images

Instead of loading a Tiled map file images can be loaded using the **-img** command line option.

To set map properties the following command line switches are available:
* **-col**=#,#,#,# Sets up to 4 fixed colors for the graphics mode
* **-meta**=width,height Sets the size of meta tiles
* **-screenBits**=# Set the number of bits for each screen byte
* **-colorBits**=# Set the number of bits for each color byte
* **-metaMapBits**=# Set the number of bits for the map data if meta tiles are used
* **-lookupBits**=# Set number of bits for looking up screen and color meta tiles for combined tiles
* **-type**=Mode where Mode is one of:
    * Text
    * TextMC*
    * ECBM
    * Bitmap
    * BitmapMC*
* **-exp**=filename, export the binary data to the destination
* **-stats**=filename, save the stats to a file

Usage:
    TiledC64 -img \<image\> \<additional switches\>

Example:

    TiledC64 -img c:\code\sk64\assets\pridemore.png -col=4,15,9,1 -meta=2,2 -colorBits=4 -type=ECBM -export=pridemore -stats=pridemoor.txt
