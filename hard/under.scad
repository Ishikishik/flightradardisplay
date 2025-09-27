
//頭4.8(2.4)(2.9)mmネジ3.4(1.7)mmナット5.8(2.9)(3.4)mm
//頭4.8(2.4)(2.9)mmネジ3.4(1.7)mmナット5.8(2.9)(3.4)mm
difference() {

    // 外枠（厚み6.5mm）
    translate([0,0,0])
    cube([97, 64,11.8 ], center = true);//外装


    //本体部
    translate([0,0,1.5])
    cube([90, 51,8.8], center = true);

    translate([90/2-21.4,-(64/2)+10,0])
    cube([6.5, 3,100], center = true);//端子

    translate([90/2-24.9,-(64/2)+32.5,0])
    cube([10.4, 11,100], center = true);//ロータリースイッチ

    translate([90/2-28.5,64/2,3.1])
    cube([19, 30,5.7], center = true);//アンテナ


    //ネジ
    translate([95/2-3,60/2-1,-20])
    cylinder(h = 100, r = 1.7, center = false);

    translate([-(95/2-3),60/2-1,-20])
    cylinder(h = 100, r = 1.7, center = false);

    translate([95/2-3,-(60/2-1),-20])
    cylinder(h = 100, r = 1.7, center = false);

    translate([-(95/2-3),-(60/2-1),-20])
    cylinder(h = 100, r = 1.7, center = false);

        //ネジ頭
    translate([95/2-3,60/2-1,-(11.8/2)])
    cylinder(h = 2, r = 3, center = false);

    translate([-(95/2-3),60/2-1,-(11.8/2)])
    cylinder(h = 2, r = 3, center = false);

    translate([95/2-3,-(60/2-1),-(11.8/2)])
    cylinder(h = 2, r = 3, center = false);

    translate([-(95/2-3),-(60/2-1),-(11.8/2)])
    cylinder(h = 2, r = 3, center = false);



   
}