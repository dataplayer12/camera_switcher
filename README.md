# camera_switcher
Gstreamer application for viewing IP cameras

## Step 1
Configure the streams variable in `main.c`. You can add any number of streams.

## Step 2
```Shell
chmod +x compile.sh
./compile.sh # You need to have gstreamer installed for compilation to be successful
```

## Step 3

```Shell
./camera_switcher
```

## Step 4
The application will initially open up in a small window. You can click anywhere in that window to go to the next camera. When you click the stream for the next camera will open in a full sized window. The first small window is therefore only for choosing the stream, not for viewing.

