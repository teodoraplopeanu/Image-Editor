# Image Editor

An image editor that performs various operations on NetPBM format files. The editor supports grayscale and color images, with functionalities to load, edit, and save images.

## NetPBM Image Overview

OA NetPBM image contains:

* A magic number (P1 to P6) indicating image type (black and white, grayscale, or color) and storage method (text or binary).
* Width and height of the image (in pixels).
* The maximum pixel value.
* The pixel data for the image.
  
*Note*: In color formats (P3 or P6), each pixel has three values representing the RGB color channels.

## Features & Commands

## LOAD < fisier >

Loads a PGM or PPM image file into memory. If an image is already loaded, it frees the memory before loading the new image. Supports text or binary files based on the magic number.
Success: Loaded <filename>
Failure: Failed to load <filename>

## SELECT < x1 > < y1 > < x2 > < y2 >

Selects a region of the image for further operations. If invalid coordinates are provided, it displays: Invalid set of coordinates.


## SELECT ALL

Resets the selection to cover the entire image.

## HISTOGRAM < x > < y >

Displays the histogram of the grayscale image using x stars and y bins.


## EQUALIZE

Equalizes a grayscale image to improve its contrast.


## ROTATE < angle >

Rotates the selected area by a given angle (multiple of 90 degrees).


 ## CROP
 Crops the image to the selected area.


## APPLY < PARAMETER >

Applies a filter (EDGE, SHARPEN, BLUR, or GAUSSIAN_BLUR) to the image. Only applicable to color images.


## SAVE < file_name > [ascii]

Saves the current image in either text or binary format based on the ascii parameter.

## EXIT

 Frees all resources and exits the program.
