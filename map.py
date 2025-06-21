import cv2
import numpy as np
import matplotlib.pyplot as plt

# 画像の読み込みとリサイズ
img = cv2.imread("map.png", cv2.IMREAD_GRAYSCALE)
img_resized = cv2.resize(img, (320, 240))

# 二値化
_, thresh = cv2.threshold(img_resized, 127, 255, cv2.THRESH_BINARY_INV)

# 輪郭抽出
contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

# 一番大きな輪郭を選択
contour = max(contours, key=cv2.contourArea)

# 輪郭を直線近似（形状を保ったまま単純化）
epsilon = 0.002 * cv2.arcLength(contour, True)  # ← 精度調整パラメータ（小さいほど精度高）
approx = cv2.approxPolyDP(contour, epsilon, True)

# 直線データの抽出 (x0, y0, x1, y1)
line_segments = []
for i in range(len(approx)):
    x0, y0 = approx[i][0]
    x1, y1 = approx[(i+1)%len(approx)][0]
    line_segments.append([int(x0), int(y0), int(x1), int(y1)])

# 近似結果の描画
output_img = np.ones((240, 320, 3), dtype=np.uint8) * 255  # 白背景
for x0, y0, x1, y1 in line_segments:
    cv2.line(output_img, (x0, y0), (x1, y1), (0, 255, 0), 1)  # 緑色の線

# 結果表示
plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.title("Thresholded")
plt.imshow(thresh, cmap='gray')
plt.subplot(1, 2, 2)
plt.title("Approximated Shape")
plt.imshow(cv2.cvtColor(output_img, cv2.COLOR_BGR2RGB))
plt.show()

# 結果確認用（任意）
print("Line segments (first 5):", line_segments[:5])
print("Total number of lines:", len(line_segments))

# 線分リストと画像の出力（必要に応じてファイル保存も可）
print(f"\n全体の線分数: {len(line_segments)}")

# C++配列として保存
with open("mapdata.hpp", "w") as f:
    f.write(f"const int NUM_LINES = {len(line_segments)};\n")
    f.write("const int lines[NUM_LINES][4] = {\n")
    for i, (x0, y0, x1, y1) in enumerate(line_segments):
        comma = "," if i < len(line_segments) - 1 else ""
        f.write(f"  {{{x0}, {y0}, {x1}, {y1}}}{comma}\n")
    f.write("};\n")

print("lines.hpp を出力しました。")