clear all
close all

function write_f64 (filename, data)
  
  fid = fopen(filename,"wb");
  
  if fid == -1
    error("couldn't open file");
  end

  fwrite(fid, data, "double");
endfunction

script_dir = fileparts(mfilename("fullpath"));
out_dir = fullfile(script_dir, "out");
if ~exist(out_dir, "dir")
  mkdir(out_dir);
endif

sinf = @(t,T) sin(2*pi*t./T);

N = 512;
M = 1024;

x2 = linspace(0,10, N);
f2 = sinf(x2,5) + 1.5*sinf(x2+0.75,2)+0.2*x2+ 0.2*sinf(x2,0.3).*10.*exp(-x2/2)+0.4*x2;

x3 = linspace(0, 10, M);
f3 = sinf(x3,5) + 0.5*sinf(x3-2,3)+0.3*x3+0.8*sinf(x3,2.3).*10.*exp(-x3/2)-0.7*x3;

img = f2 .* f3';

size(img)

figure
imshow(img,[]);
title("image")

img_name = strcat("image_", num2str(N), "x", num2str(M), ".f64")

write_f64(fullfile(out_dir, img_name), img(:));
