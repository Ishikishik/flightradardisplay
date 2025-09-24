

//頭4.8(2.4)(2.9)mmネジ3.4(1.7)mmナット5.8(2.9)(3.4)mm
difference() {
    // 外枠（厚み6.5mm）
    cube([97, 62,15 ], center = true);//外装

    //ディスプレイ部
    translate([0,0,-7.5])
    cube([69.5, 51,9], center = true);

    //本体部
    translate([0,0,7.5])
    cube([90, 51,21], center = true);

    //アンテナ部
    translate([0,0,7.5])
    cube([69.5, 51,4], center = true);

    //横端子部
    translate([42.5,0,0])
    cube([5, 51,10], center = true);


    //ネジ
    translate([95/2-3,60/2-2,0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([-(95/2-3),60/2-2,0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([95/2-3,-(60/2-2),0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([-(95/2-3),-(60/2-2),0])
    cylinder(h = 10, r = 1.7, center = false);



   
}



/*
//trancerate([0,0,0]){cube(size=[105.5,46,1.5] , center=true);}//基盤サイズ)
//頭4.8(2.4)(2.9)mmネジ3.4(1.7)mmナット5.8(2.9)(3.4)mm
difference() {
    // 外枠（厚み6.5mm）
    union(){
    cube([110.5, 60, 6.5], center = true);//外装
    translate([-110.5/2, 0, 2.5])
    rotate([0,90,0])
        cylinder(h = 2.5, r = 20, center = false);//同軸固定ネジ穴
    }

    // 内側をくり抜き（深さは 6.5 - 底の厚み）
    translate([0, 0, 5/2])  // 底を 0.5mm 残す
        cube([105.5, 46, 1.5], center = true);//基盤
    translate([0, 0, 2.5])  // 底を 0.5mm 残す
        cube([110.5, 5, 5], center = true);//端子部分
    translate([0, 0, 0])  // 底を 0.5mm 残す
        cube([100, 30, 6.5], center = true);//底面穴
    translate([0, 30, 0])  // 底を 0.5mm 残す
        cube([95, 10, 9], center = true);//側面
    translate([0, -30, 0])  // 底を 0.5mm 残す
        cube([95, 10, 9], center = true);//側面

    translate([110.5/2-4,60/2-3.5, -6.5/2])
        cylinder(h = 10, r = 1.7, center = false);//ネジ
    translate([110.5/2-4,-(60/2-3.5), -6.5/2])
        cylinder(h = 10, r = 1.7, center = false);//ネジ
    translate([-(110.5/2-4),60/2-3.5, -6.5/2])
        cylinder(h = 10, r = 1.7, center = false);//ネジ
    translate([-(110.5/2-4),-(60/2-3.5), -6.5/2])
        cylinder(h = 10, r = 1.7, center = false);//ネジ
    translate([-(110.5/2-4),-(60/2-3.5), -6.5/2-8])
        cylinder(h=10, r=3.4, $fn=6);//ネジ頭下ナット
    translate([-(110.5/2-4),(60/2-3.5), -6.5/2-8])
        cylinder(h=10, r=3.4, $fn=6);//ネジ頭下ナット
    translate([(110.5/2-4),-(60/2-3.5), -6.5/2-8])
        cylinder(h=10, r=3.4, $fn=6);//ネジ頭下ナット
    translate([(110.5/2-4),(60/2-3.5), -6.5/2-8])
        cylinder(h=10, r=3.4, $fn=6);//ネジ頭下ナット

    
}
*/