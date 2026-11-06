

//頭4.8(2.4)(2.9)mmネジ3.4(1.7)mmナット5.8(2.9)(3.4)mm
difference() {
    // 外枠（厚み6.5mm）
    cube([97, 64,15 ], center = true);//外装

    //ディスプレイ部
    translate([-3,0,-7.5])
    cube([72, 51,9], center = true);

    //本体部
    translate([0,0,7.5])
    cube([90, 51,21], center = true);

    //アンテナ部
    translate([0,0,7.5])
    cube([69.5, 51,4], center = true);

    //横端子部
    translate([40,0,0])
    cube([10, 51,12], center = true);


    //ネジ
    translate([95/2-3,60/2-1,0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([-(95/2-3),60/2-1,0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([95/2-3,-(60/2-1),0])
    cylinder(h = 10, r = 1.7, center = false);

    translate([-(95/2-3),-(60/2-1),0])
    cylinder(h = 10, r = 1.7, center = false);



   
}



