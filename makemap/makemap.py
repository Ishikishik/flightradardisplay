import matplotlib.pyplot as plt
import cartopy.crs as ccrs
import cartopy.feature as cfeature


# 座標範囲（緯度・経度）
lon_min, lon_max = 135.755, 142.954
lat_min, lat_max = 33.451, 39.865

# 図と軸の作成
fig = plt.figure(figsize=(4, 3), dpi=150)
ax = plt.axes(projection=ccrs.PlateCarree())

# アスペクト比を自動に（縦横比は気にしない）
ax.set_aspect('auto')

# 範囲指定（地図をめいっぱい表示）
ax.set_extent([lon_min, lon_max, lat_min, lat_max], crs=ccrs.PlateCarree())

# グリッドやラベル非表示
ax.axis('off')  # 枠も消す

# 地図データ描画
ax.add_feature(cfeature.LAND, facecolor='black')  # 陸地（色は黒にしておくと蛍光緑との対比に便利）
ax.add_feature(cfeature.OCEAN, facecolor='white') # 海
ax.add_feature(cfeature.COASTLINE, linewidth=0.5, edgecolor='gray')

# 保存
plt.savefig("map.png", bbox_inches='tight', pad_inches=0)
plt.close()