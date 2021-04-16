# ds4ambi

Ambilight-like plugin to use with ds4 lightbar.  
Works on PSTV or Vita with minivitatv

## Usage

There are two versions:  
1. User plugin (.suprx) - this works **only** with homebrew, add it under relevant app id in your ur0:/tai/config.txt
2. Kernel plugin (.skprx) - this works always and with everything, including shell (livearea) and built-in apps like video player. Add it under `*KERNEL`.

## Bugs and limitations

It uses dominant color algo, but not every screen pixel is used in calculations due to performance reasons.  
This may lead to slight led flashing on some images.

ds4 led is meh, and can't show full RGB spectrum.


## Thanks

* CPBS Discord server
* Graphene, cuevavirus, Princess-of-Sleeping, S1ngyy, SonicMastr - for support
* MERLev, xerpi - for code samples
