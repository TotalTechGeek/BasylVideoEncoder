# Basyl Video Encoder
An experimental video encoder. Written in an independent study course during High School.  
### What is this?
This project features a [video encoder](https://en.wikipedia.org/wiki/Encoder). It takes data from your video stream and makes it smaller. The video was designed for academic purposes rather than industry purposes. [H.264](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC) and Google's recent [VP9](https://en.wikipedia.org/wiki/VP9) projects achieve superior compression ratios. The current implementation features the [Fourier Transform](https://en.wikipedia.org/wiki/Fourier_transform), which is a function used to analyze frequencies,  to compress your data. The Fourier Transform, compared to the standard [Discrete Cosine Transform](https://en.wikipedia.org/wiki/Discrete_cosine_transform) has the unique property of being able to ridiculously scale quality down and still achieve recognizable images.

In the image you see below you can see that the quality scaled by an enormous factor. Showing a compressed  image frame using  normal settings would be a little redundant as there is no visible difference to the real image.

Intentional Crazy Quality Scale Down =>
![Me in an insanely compressed image](http://i.imgur.com/eZSoY2g.jpg)

On normal settings, the algorithm achieves ratios comparable to older versions of [MPEG](https://en.wikipedia.org/wiki/Moving_Picture_Experts_Group), with quality slightly superior to MPEG

The video encoder allowed me to implement a Video Chat service similar to [Skype](http://www.skype.com/en/). 

### How does it work?
First the [frame](https://en.wikipedia.org/wiki/Still_frame) is analyzed for similar sub-frames. If the [sub-frames](https://en.wikipedia.org/wiki/Inter_frame) have barely changed, they are culled. The remaining frames are converted to the [YCbCr color channels](https://en.wikipedia.org/wiki/YCbCr) from [RGB](https://en.wikipedia.org/wiki/RGB_color_model). On the remaining frames have the Fourier Transform (or DCT) performed on the data. The Fourier Transform is more compressible than the RGB values.

This is all performed in a buffer, which you can then export yourself. In order to keep the DIY feel to the project, you should add your own compressor to exporting the buffer. I used [miniz.c](https://code.google.com/p/miniz/).

### Why did you make this?
I wanted to use VP8 from Google, but I couldn't build it properly without setting up a bunch of dependencies. After setting up the dependencies (finally), I found that their library made it difficult to do anything other than encode directly to a file. Since I needed this for a networking algorithm, I ditched VP8 and tried [FFMPEG](https://www.ffmpeg.org/), and ran into the same issues.

That's when I decided to learn video and image compression, and the technology behind it. Hopefully this can be utilized for academic purposes by someone else.

### Why Fourier Transform?
Because I couldn't figure out the scale factors for the DCT using the [FFTW libraries](http://www.fftw.org/) at first. And when I did figure out how to use the DCT (which does get slightly better performance), it wasn't able to scale quality down as much as the Fourier Transform.

Editing just a few lines of code will make this use the DCT, or other kinds of transforms. 


### What's that blue in the image up there?
My friend had a blue shirt that day. So blue, that mathematically it wasn't converted to grayscale. I set the scale factors up as high as I could for that video, and as you can imagine, it compressed pretty well. 
