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

x1 = linspace(0,10,1024);
f1 = sinf(x1,5) + 1.5*sinf(x1+0.75,2)+0.1*x1;

write_f64(fullfile(out_dir, "data1.f64"), f1);

figure
plot(x1,f1)
title("data 1")

x2 = linspace(0,10,503);
f2 = sinf(x2,5) + 1.5*sinf(x2+0.75,2)+0.2*x2+ 0.2*sinf(x2,0.3).*10.*exp(-x2/2)+0.4*x2;

write_f64(fullfile(out_dir, "data2.f64"), f2);

figure
plot(x2,f2);
title("data 2")
