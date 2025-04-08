%% Calculate Third Octave Bands (base 2) in Matlab
fcentre  = 10^3 * (2 .^ ([-18:13]/3))
fd = 2^(1/6);
fupper = fcentre * fd
flower = fcentre / fd

%% Calculate Third Octave Bands (base 10) in Matlab
fcentre = 10.^(0.1.*[12:43])
fd = 10^0.05;
fupper = fcentre * fd
flower = fcentre / fd