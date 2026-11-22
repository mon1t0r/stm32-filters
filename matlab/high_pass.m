fs = 16000;
N = 16000;

Nb = 160;
f0 = 800 / (fs / 2);
b = fir1(Nb, f0, 'high');

% Print filter coefficients
fprintf('%.15g, ', b);
fprintf('\n');

freqz(b, 1, N, fs);

% Verification
ts = 1 / fs;
t = (0:N-1)*ts;
df = (fs/N);
f = (0:N-1)*df;

X = 0;
for n = 1:79
    X = X + sin(2*pi*n*50*t);
end
% 16-bit uniform quantization
X = round((X + 1)*32767.5)/32767.5 - 1;

figure;
plot(t, X);
xlabel('Time (s)');
ylabel('Amplitude');
title('Signal');
grid on;

XF = filter(b, 1, X);

figure;
plot(t, XF);
xlabel('Time (s)');
ylabel('Amplitude');
title('Signal - Filtered');
grid on;

Y = 2 * abs(fft(X)) / N;

figure;
plot(f, Y);
ylim([0.0 1.0]);
xlabel("Freq (Hz)");
ylabel("Amplitude");
title('Spectrum');
grid on;

YF = 2 * abs(fft(XF)) / N;

figure;
plot(f, YF);
ylim([0.0 1.0]);
xlabel("Freq (Hz)");
ylabel("Amplitude");
title('Spectrum - Filtered');
grid on;
