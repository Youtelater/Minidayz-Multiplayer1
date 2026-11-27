#pragma once
#include "minidayz/generated/source_box_simple_drops.hpp"
#include "minidayz/source_interior_container.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/original_initial_building_loot.hpp"
#include <cmath>
#include <memory>
#include <new>
#include <random>
namespace minidayz::items {
// Original Game_events 389950912936355, normal (non-map_fs-extra) branches.
// Original unlock variables initialize to zero; these branches use that state
// until achievement unlock state is connected. No synthetic random item pool.
inline bool source_box_default_nested_drops(std::uint64_t category,
    std::mt19937& random,GroundLootSystem& ground,float x,float y) noexcept {
    const auto pick=[&](const auto& a) noexcept {
        return a[source_initial_loot_choice_index(random(),sizeof(a)/sizeof(a[0]))];
    };
    const auto drop=[&](std::uint16_t item,int dx,int dy) noexcept {
        const auto id=source_initial_item_after_default_unlocks(item);
        return id && source_item_exists(id) &&
            ground.add({id,1,kUseSourceItemCondition},x+dx,y+dy);
    };
    switch(category){
    case 51: { // 655155288571769
        constexpr int pool[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
        int randy=pick(pool);
        if(randy==1) { // 224628769285970
            if(!drop(12,10,5))return false; // 341072457540630
            if(!drop(16,-10,5))return false; // 138172642090691
            if(!drop(69,0,10))return false; // 841738167996245
            if(!drop(100,0,-10))return false; // 158336073542787
            return true;
        }
        if(randy==2) { // 577503117603168
            if(!drop(14,10,5))return false; // 330239042677980
            if(!drop(15,-10,5))return false; // 395700614071643
            if(!drop(11,0,10))return false; // 570821553775105
            if(!drop(13,10,-5))return false; // 898500240411734
            if(!drop(13,-10,-5))return false; // 428379451772826
            if(!drop(138,10,-5))return false; // 907449288286404
            if(!drop(138,-10,-5))return false; // 727882003143457
            if(!drop(14,0,-10))return false; // 451864189227711
            return true;
        }
        if(randy==3) { // 229243283012988
            if(!drop(13,10,5))return false; // 568900972104273
            if(!drop(13,-10,5))return false; // 772278977211026
            if(!drop(138,10,-5))return false; // 946189700239626
            if(!drop(138,-10,-5))return false; // 823665232915735
            if(!drop(14,0,10))return false; // 278885534088386
            if(!drop(14,10,-5))return false; // 905993740088333
            if(!drop(15,-10,-5))return false; // 315864404341686
            if(!drop(15,0,-10))return false; // 798639111631180
            return true;
        }
        if(randy==4) { // 761452656206848
            if(!drop(29,10,5))return false; // 245068389315024
            if(!drop(710,-10,5))return false; // 128947333566913
            if(!drop(708,-10,5))return false; // 659002315176998
            if(!drop(702,-10,5))return false; // 971294095925664
            return true;
        }
        if(randy==5) { // 590327086533953
            if(!drop(39,10,5))return false; // 349592499037026
            if(!drop(39,-10,5))return false; // 338674932866822
            if(!drop(36,0,10))return false; // 662056180769272
            return true;
        }
        if(randy==6) { // 283020191578477
            if(!drop(35,10,5))return false; // 258691435905951
            if(!drop(39,-10,5))return false; // 221320316964194
            return true;
        }
        if(randy==7) { // 182792820248994
            if(!drop(56,10,5))return false; // 678819495826661
            if(!drop(21,-10,5))return false; // 426956334356787
            if(!drop(119,0,10))return false; // 176717040815973
            return true;
        }
        if(randy==8) { // 186264085228139
            if(!drop(55,-10,5))return false; // 891404416437224
            if(!drop(20,0,10))return false; // 795316610516030
            return true;
        }
        if(randy==9) { // 365929261561048
            if(!drop(263,-10,5))return false; // 206716667173676
            if(!drop(3,10,5))return false; // 668431279508805
            return true;
        }
        if(randy==10) { // 509873148243044
            if(!drop(251,-10,5))return false; // 278520638889606
            if(!drop(3,10,5))return false; // 164334425275235
            return true;
        }
        if(randy==11) { // 487852413921679
            if(!drop(34,10,5))return false; // 331717289972571
            if(!drop(24,-10,5))return false; // 179975116596132
            if(!drop(25,0,10))return false; // 363622328816209
            return true;
        }
        if(randy==12) { // 304163162015679
            if(!drop(400,-10,5))return false; // 273785213033424
            return true;
        }
        if(randy==13) { // 703740455302062
            if(!drop(401,-10,5))return false; // 109611386005626
            return true;
        }
        if(randy==14) { // 934827794311758
            if(!drop(105,10,5))return false; // 733585619737365
            return true;
        }
        if(randy==15) { // 135953224917191
            if(!drop(106,10,5))return false; // 183947583068688
            return true;
        }
        if(randy==16) { // 332732062276886
            if(!drop(173,0,5))return false; // 623696350210612
            if(!drop(12,10,5))return false; // 561092114678947
            if(!drop(12,-10,5))return false; // 779252381839352
            return true;
        }
        if(randy==17) { // 499281199478444
            if(!drop(169,0,5))return false; // 475534827298404
            if(!drop(69,10,5))return false; // 237168415623762
            if(!drop(69,-10,5))return false; // 329918214166962
            return true;
        }
        if(randy==18) { // 879603291390376
            if(!drop(161,0,5))return false; // 953970195491000
            if(!drop(15,10,5))return false; // 115540935596528
            if(!drop(15,-10,5))return false; // 531558288708713
            return true;
        }
        if(randy==19) { // 664260007289222
            if(!drop(111,10,5))return false; // 504525317887341
            return true;
        }
        if(randy==20) { // 291649104694131
            if(!drop(112,10,5))return false; // 814438928463793
            return true;
        }
        if(randy==21) { // 660329834477926
            if(!drop(119,10,5))return false; // 798050248789640
            if(!drop(21,-10,5))return false; // 284557576876317
            return true;
        }
        if(randy==22) { // 431779808776797
            if(!drop(124,10,5))return false; // 382932873932608
            return true;
        }
        if(randy==23) { // 930866247970766
            if(!drop(153,10,5))return false; // 465688157819537
            if(!drop(16,-10,5))return false; // 549915918372866
            return true;
        }
        if(randy==24) { // 130615969376544
            if(!drop(53,10,5))return false; // 591294292656169
            if(!drop(53,-10,5))return false; // 267690034932547
            return true;
        }
        return false;
    }
    case 52: { // 527994098352019
        constexpr int pool[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,29,30};
        int randy=pick(pool);
        if(randy==15) { // 499068344635560 aug_unlock
            constexpr int retry[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21}; randy=pick(retry);
        }
        if(randy==16) { // 451188469757604 mac10_unlock
            constexpr int retry[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21}; randy=pick(retry);
        }
        if(randy==17) { // 901362518451381 saiga12_unlock
            constexpr int retry[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21}; randy=pick(retry);
        }
        if(randy==19) { // 813083311525689 sv98_unlock
            constexpr int retry[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21}; randy=pick(retry);
        }
        if(randy==29) { // 988273294794662 vss_unlock
            constexpr int retry[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,20,21}; randy=pick(retry);
        }
        if(randy==1) { // 520511069647202
            if(!drop(46,10,5))return false; // 979055932262915
            if(!drop(28,-10,5))return false; // 651661791407113
            return true;
        }
        if(randy==2) { // 805269486271720
            if(!drop(29,10,5))return false; // 399679966523401
            if(!drop(19,-10,5))return false; // 835825718677370
            return true;
        }
        if(randy==3) { // 651327478842048
            if(!drop(36,10,5))return false; // 316008873280536
            if(!drop(39,-10,5))return false; // 642517530932209
            if(!drop(35,0,10))return false; // 389637830351896
            return true;
        }
        if(randy==4) { // 684392900181342
            if(!drop(55,-10,5))return false; // 735705353742115
            if(!drop(43,0,5))return false; // 131741824792972
            return true;
        }
        if(randy==5) { // 702845354857274
            if(!drop(76,10,5))return false; // 246030833407942
            return true;
        }
        if(randy==6) { // 371744567717729
            if(!drop(77,10,5))return false; // 894395481920946
            return true;
        }
        if(randy==7) { // 650040066615148
            if(!drop(78,10,5))return false; // 679981102970303
            return true;
        }
        if(randy==8) { // 965423007796566
            if(!drop(17,10,5))return false; // 130598157684298
            if(!drop(83,-10,5))return false; // 963061598001837
            if(!drop(17,0,10))return false; // 973138909500164
            return true;
        }
        if(randy==9) { // 581107264664527
            if(!drop(168,0,5))return false; // 404794170010330
            if(!drop(13,10,5))return false; // 499870582735253
            if(!drop(13,-10,5))return false; // 920045739026960
            return true;
        }
        if(randy==10) { // 617335203441160
            if(!drop(251,0,10))return false; // 212330747774503
            if(!drop(402,0,10))return false; // 450396738410996
            return true;
        }
        if(randy==11) { // 532071944305238
            if(!drop(403,0,10))return false; // 573179221405793
            return true;
        }
        if(randy==12) { // 793781925993314
            if(!drop(400,0,10))return false; // 754074714461023
            if(!drop(251,0,10))return false; // 648253196691250
            return true;
        }
        if(randy==13) { // 847365527549426
            if(!drop(451,0,10))return false; // 690874205763255
            return true;
        }
        if(randy==14) { // 739448180098924
            if(!drop(457,0,10))return false; // 642940001610215
            return true;
        }
        if(randy==15) { // 854163761469334
            if(!drop(177,0,5))return false; // 358444916837261
            if(!drop(11,10,5))return false; // 365094560800317
            if(!drop(11,-10,5))return false; // 305135979400683
            return true;
        }
        if(randy==16) { // 241923305762439
            if(!drop(178,0,5))return false; // 337958274230843
            if(!drop(17,10,5))return false; // 386583859665113
            if(!drop(17,-10,5))return false; // 903434207133880
            return true;
        }
        if(randy==17) { // 750087626877265
            if(!drop(179,0,5))return false; // 149250551341124
            if(!drop(69,10,5))return false; // 197136260367743
            if(!drop(69,-10,5))return false; // 563707850493946
            return true;
        }
        if(randy==18) { // 155999400131315
            if(!drop(185,0,5))return false; // 500694993192033
            if(!drop(101,10,5))return false; // 212268811735441
            if(!drop(101,-10,5))return false; // 784124134297597
            return true;
        }
        if(randy==19) { // 764689319510755
            if(!drop(186,0,5))return false; // 161178717396196
            if(!drop(13,10,5))return false; // 795420655019705
            if(!drop(13,-10,5))return false; // 629368734035074
            return true;
        }
        if(randy==20) { // 919955499650716
            if(!drop(85,0,10))return false; // 831899605495167
            return true;
        }
        if(randy==21) { // 997634851585064
            if(!drop(99,0,10))return false; // 240581781339691
            if(!drop(99,0,-10))return false; // 612954614524371
            if(!drop(119,0,-10))return false; // 156192264360261
            return true;
        }
        if(randy==22) { // 795275896664899
            if(!drop(184,0,10))return false; // 141301897953175
            if(!drop(101,0,-10))return false; // 895697762062266
            if(!drop(101,0,-10))return false; // 798926092280535
            return true;
        }
        if(randy==23) { // 639348120064537
            if(!drop(162,0,10))return false; // 269344811346881
            if(!drop(11,0,-10))return false; // 624901454171790
            if(!drop(11,0,-10))return false; // 927027906725834
            return true;
        }
        if(randy==24) { // 733005784480654
            if(!drop(188,0,10))return false; // 273081619652494
            if(!drop(15,10,5))return false; // 714862407771073
            if(!drop(15,-10,5))return false; // 145875892580353
            return true;
        }
        if(randy==25) { // 575631532380424
            if(!drop(159,0,10))return false; // 148825356158586
            if(!drop(14,0,-10))return false; // 145476592143020
            if(!drop(14,0,-10))return false; // 952756605624325
            return true;
        }
        if(randy==26) { // 368351889378706
            if(!drop(106,0,10))return false; // 779971889441691
            if(!drop(108,0,10))return false; // 369764677742885
            return true;
        }
        if(randy==27) { // 757981211909566
            if(!drop(105,0,10))return false; // 630925840077002
            if(!drop(107,0,10))return false; // 190352957010369
            return true;
        }
        if(randy==28) { // 814240079907708
            if(!drop(230,0,10))return false; // 866710583333903
            if(!drop(105,0,10))return false; // 482702966956706
            if(!drop(105,0,10))return false; // 846224277794608
            return true;
        }
        if(randy==29) { // 463410724486494
            if(!drop(240,0,5))return false; // 453759969703972
            if(!drop(101,10,5))return false; // 475072160507233
            if(!drop(101,-10,5))return false; // 633080415759642
            return true;
        }
        if(randy==30) { // 815565865215371
            if(!drop(747,0,5))return false; // 629638495560392
            if(!drop(747,-10,5))return false; // 455930880788884
            return true;
        }
        return false;
    }
    case 53: { // 483637851273824
        constexpr int pool[]={1,2,3,4,5,6,7,8,9,11,12,13,14,15,16};
        int randy=pick(pool);
        if(randy==1) { // 115783717796652
            if(!drop(80,0,5))return false; // 899349381162600
            if(!drop(15,10,5))return false; // 201893392791397
            return true;
        }
        if(randy==2) { // 547694938580643
            if(!drop(81,0,5))return false; // 635065904101770
            if(!drop(11,10,5))return false; // 465030475767658
            return true;
        }
        if(randy==3) { // 205332696864288
            if(!drop(75,0,5))return false; // 992361324510567
            return true;
        }
        if(randy==4) { // 907301153857415
            if(!drop(98,0,5))return false; // 209161515677782
            return true;
        }
        if(randy==5) { // 297048731204213
            if(!drop(404,0,5))return false; // 873235941059213
            return true;
        }
        if(randy==6) { // 455206419462981
            if(!drop(452,0,5))return false; // 709366650516422
            return true;
        }
        if(randy==7) { // 553374447718294
            if(!drop(303,10,5))return false; // 564851184431849
            if(!drop(355,-10,5))return false; // 543090562288721
            return true;
        }
        if(randy==8) { // 763857312083281
            if(!drop(175,10,5))return false; // 130497279234748
            if(!drop(138,-10,5))return false; // 564074101617017
            if(!drop(138,-10,5))return false; // 708130247788404
            return true;
        }
        if(randy==9) { // 231387349542860
            if(!drop(174,10,5))return false; // 305470938396954
            if(!drop(14,-10,5))return false; // 736477688922853
            if(!drop(14,-10,5))return false; // 300959202046513
            return true;
        }
        if(randy==11) { // 335226253284849
            if(!drop(89,0,5))return false; // 424839868650498
            return true;
        }
        if(randy==12) { // 448535878520488
            if(!drop(90,0,5))return false; // 258255796526338
            return true;
        }
        if(randy==13) { // 559072491946875
            if(!drop(85,0,5))return false; // 793345970247948
            if(!drop(85,0,-5))return false; // 833298410764024
            if(!drop(85,0,-5))return false; // 139999240133864
            return true;
        }
        if(randy==14) { // 375938571757071
            if(!drop(103,0,5))return false; // 909348159268146
            return true;
        }
        if(randy==15) { // 978225039172451
            if(!drop(113,0,5))return false; // 417706006481712
            return true;
        }
        if(randy==16) { // 143624916816273
            if(!drop(761,0,5))return false; // 321439349340410
            if(!drop(36,0,5))return false; // 719187777975704
            return true;
        }
        return false;
    }
    case 999: { // 957033466883016
        constexpr int pool[]={1,2,3};
        const int randy=pick(pool);
        if(randy==1) { // 210336597575079
            if(!drop(242,10,5))return false; // 683391297369540
            if(!drop(758,-10,5))return false; // 405904378927929
            if(!drop(758,-10,5))return false; // 909893986306895
            return true;
        }
        if(randy==2) { // 793065042020001
            if(!drop(243,10,5))return false; // 997757746526117
            if(!drop(462,-10,5))return false; // 434812818530632
            return true;
        }
        if(randy==3) { // 953292671901276
            if(!drop(245,10,5))return false; // 865378487804537
            return true;
        }
        return false;
    }
    default:return false;
    }
}
// Original direct Create + Set instance variable 1 branches (5–13).
// Object-to-item mapping is from Spawn_drop event 5418835019314848.
struct SourceBoxDirectQuantityDrop {
    std::uint16_t category,object,item,count;
    int dx,dy;
    std::uint64_t create_uid,set_uid;
};
inline constexpr SourceBoxDirectQuantityDrop kSourceBoxDirectQuantityDrops[]={
    {5,30,11,30,-10,0,830489321749565,551147352530324},
    {5,30,11,30,10,0,543615434550665,710078025218332},
    {6,29,12,30,-10,0,223423840006814,980517091613862},
    {6,29,12,30,10,0,133365826261222,841318704621879},
    {7,31,13,15,-10,0,847263439637079,638707055115389},
    {7,31,13,15,10,0,402118452268962,647826333530370},
    {8,58,14,30,-10,0,638304838800480,784408970424703},
    {8,58,14,30,10,0,413794190431448,980561959487246},
    {9,57,15,30,-10,0,974852921856562,874749350342166},
    {9,57,15,30,10,0,814700281272083,572092384746227},
    {10,59,16,20,-10,0,331026146732388,940912535499435},
    {10,59,16,20,10,0,791771551783665,213338620916612},
    {11,28,17,15,-10,0,594730930621676,256686527940780},
    {11,28,17,15,10,0,284639587721876,897141181464374},
    {12,444,58,50,-10,0,573726606301362,144638394945628},
    {12,444,58,50,10,0,158583749742755,533311061582807},
    {13,566,69,30,-10,0,221752971392365,583427760301081},
    {13,566,69,30,10,0,489097352903775,505664598798911},
    {13,566,69,30,0,5,201300767367609,462861427093768},
};
inline bool source_box_direct_quantity_drops(std::uint64_t category,
    GroundLootSystem& ground,float x,float y) noexcept {
    bool found=false;
    for(const auto& row:kSourceBoxDirectQuantityDrops){
        if(row.category!=category)continue;
        found=true;
        if(!ground.add({row.item,row.count,kUseSourceItemCondition},
                      x+row.dx,y+row.dy))return false;
    }
    return found;
}
// Ordered mixed Spawn_drop / direct Create + variable 1 source actions.
// object=0 denotes Spawn_drop; direct Create bypasses Spawn_drop unlock swaps.
inline constexpr SourceBoxDirectQuantityDrop kSourceBoxMixedDrops[]={
    {14,370,35,1,-10,0,877299284863997,0},
    {14,370,35,1,10,0,900546395108623,0},
    {15,379,36,1,-10,0,327695191837222,0},
    {15,379,36,1,10,0,995355799123242,0},
    {17,36,19,1,-10,0,195022771015296,0},
    {17,36,19,1,10,0,632904144756102,0},
    {18,840,85,100,-10,0,633221652701240,387097127016838},
    {18,840,85,100,10,0,813151583535687,261774263596193},
    {18,840,85,100,10,0,964573832608017,807153139196708},
    {19,814,86,100,-10,0,515477598734361,783478953048140},
    {20,704,75,100,-10,0,862102095940145,432333493989220},
    {1011,0,19,1,0,0,731029233543557,0},
    {1011,0,28,1,0,0,304447631010900,0},
    {1011,0,63,1,0,0,412712429424378,0},
    {1011,35,18,3,0,0,692546564637758,720380020382423},
    {37,0,181,1,5,5,474645204187080,0},
    {37,137,100,25,-10,0,431077539699793,560244952914407},
    {38,0,171,1,5,5,818968242261816,0},
    {38,28,17,10,-10,0,513207209115698,290449641269750},
    {39,0,166,1,5,5,878396003274904,0},
    {39,444,58,30,-10,0,609043848455196,763497290621854},
    {40,0,153,1,5,5,168627157653623,0},
    {40,59,16,12,-10,0,938834874671906,160774076138135},
    {41,0,157,1,5,5,181533742372450,0},
    {41,58,14,15,-10,0,890221921986144,781905881339471},
    {42,0,161,1,5,5,395681608136990,0},
    {42,57,15,30,-10,0,982297695157646,651888880585313},
    {111,0,168,1,0,5,289680099826579,0},
    {111,0,18,1,-5,5,128864100850241,0},
    {111,0,24,1,0,0,407559413436207,0},
    {111,0,120,1,0,0,363316170876698,0},
    {111,31,13,20,0,5,864167434314714,738777126509320},
    {112,0,187,1,0,5,931203346329440,0},
    {112,31,13,20,0,5,294041924849817,402777771259701},
    {112,31,13,20,0,-5,161715807451926,120257528708732},
    {112,31,13,20,0,-5,630514429946739,275778090644897},
    {116,1013,120,10,0,5,119908934625811,577410144710955},
    {116,0,121,1,-10,5,949600185970119,0},
    {116,0,121,1,10,5,798037407334530,0},
    {116,0,125,1,5,-5,784662235851653,0},
    {116,0,25,1,-5,0,107333496136635,0},
};
inline bool source_box_has_mixed_drops(std::uint64_t category) noexcept {
    for(const auto& row:kSourceBoxMixedDrops)if(row.category==category)return true;
    return false;
}
inline bool source_box_mixed_drops(std::uint64_t category,
    GroundLootSystem& ground,float x,float y) noexcept {
    bool found=false;
    for(const auto& row:kSourceBoxMixedDrops){
        if(row.category!=category)continue;
        found=true;
        const auto id=row.object?row.item:source_initial_item_after_default_unlocks(row.item);
        if(!id || !source_item_exists(id) || !ground.add({id,row.count,kUseSourceItemCondition},x+row.dx,y+row.dy))return false;
    }
    return found;
}
// Original normal-map category 456789, event 658726163584589. The category's
// ordered random outcomes use initial locked achievement state (14/15 Else).
inline constexpr SourceBoxDirectQuantityDrop kSourceBox456789Drops[]={
    {12,0,174,1,0,5,398861462079100,0},
    {12,0,14,1,10,5,146295219355332,0},
    {12,0,14,1,-10,5,551327128552180,0},
    {13,0,175,1,0,5,893149045972817,0},
    {13,0,138,1,10,5,781723955011471,0},
    {13,0,138,1,-10,5,634066241558841,0},
    {14,0,158,1,0,5,493282777015798,0},
    {14,0,11,1,10,5,434356921941153,0},
    {14,0,11,1,-10,5,813385771015485,0},
    {15,0,11,1,0,5,913110060944816,0},
    {15,0,17,1,10,5,732692153195716,0},
    {15,0,17,1,-10,5,346339047402277,0},
    {1,0,184,1,0,5,520291183732474,0},
    {1,0,101,1,10,5,152513790203661,0},
    {1,0,101,1,-10,5,223432717593331,0},
    {2,0,108,1,0,10,414499083319707,0},
    {2,0,106,1,0,10,804522579856284,0},
    {2,0,106,1,0,10,798743575601141,0},
    {2,0,106,1,0,10,753678102532954,0},
    {2,0,106,1,0,10,656388998326189,0},
    {2,0,106,1,0,10,101178050857824,0},
    {3,0,107,1,0,10,704235182893082,0},
    {3,0,105,1,0,10,458842131033601,0},
    {3,0,105,1,0,10,658326342703154,0},
    {3,0,105,1,0,10,908147880136668,0},
    {3,0,105,1,0,10,912783368934764,0},
    {3,0,105,1,0,10,265303757954145,0},
    {4,0,452,1,0,0,250351282913920,0},
    {4,0,263,1,10,5,311111906331102,0},
    {4,0,303,1,-10,5,395939338594451,0},
    {5,0,124,1,10,5,455095198248382,0},
    {5,0,79,1,-10,5,903120829960514,0},
    {5,0,112,1,0,5,996653947868220,0},
    {6,0,78,1,10,5,581702899064214,0},
    {6,0,80,1,-10,5,298851829853127,0},
    {6,0,111,1,0,5,861589417187615,0},
    {7,0,404,1,0,5,493737809353893,0},
    {7,0,251,1,0,5,777618496257578,0},
    {8,0,405,1,10,5,591242576363294,0},
    {8,0,355,1,-10,5,610799829931077,0},
    {9,0,98,1,0,5,925995778238658,0},
    {9,0,83,1,0,5,962633689041463,0},
    {9,0,77,1,0,5,949293362425223,0},
    {10,0,98,1,0,5,374029276040008,0},
    {10,0,83,1,0,5,605741544645132,0},
    {10,0,77,1,0,5,482691133701271,0},
    {11,30,11,30,-10,0,988092626299262,956072888193699},
    {11,29,12,30,-10,5,799301197623447,324920121112310},
    {11,31,13,20,-10,5,221374645476034,825086525619096},
    {11,1102,138,20,-10,5,488989713525269,686496265677812},
    {11,58,14,30,-10,-10,993135438187392,374038297723459},
    {11,57,15,30,-10,10,823341456263769,865818765963292},
    {11,59,16,20,-5,0,582714301834707,916033222620745},
    {11,28,17,15,5,0,834149179428592,324848320462974},
    {11,444,58,50,0,5,348937922364929,146058815702915},
    {11,566,69,30,0,-5,595386032624080,348586693379232},
};
inline bool source_box_456789_drops(std::mt19937& random,
    GroundLootSystem& ground,float x,float y) noexcept {
    constexpr int pool[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    const auto outcome=pool[source_initial_loot_choice_index(random(),sizeof(pool)/sizeof(pool[0]))];
    bool found=false;
    for(const auto& row:kSourceBox456789Drops){
        if(row.category!=outcome)continue;
        found=true;
        const auto id=row.object?row.item:source_initial_item_after_default_unlocks(row.item);
        if(!id || !source_item_exists(id) || !ground.add({id,row.count,kUseSourceItemCondition},x+row.dx,y+row.dy))return false;
    }
    return found;
}
// Original normal category 53 event 483637851273824 has an unconditional
// final child 432922065105325 -> Save_run_progress action 970390035245631.
// The scene dispatches this only AFTER the native loot transaction succeeds.
inline constexpr bool source_box_requests_run_save(std::uint64_t category) noexcept {
    return category==53;
}
// Native transaction: never consume a source box or advance its RNG
// when its branch is not yet supported or cannot fit in ground storage.
// The actual box is selected by the original Player181 collision mask.
inline bool source_open_simple_box(WorldSystem& world,std::size_t index,
    WorldPoint actor,GroundLootSystem& ground,std::mt19937& rng) noexcept {
    const auto* box=world.placement(index);
    if(!box || !world.source_placement_active(*box) ||
       !world_source::source_bunker_lootbox_closed(*box) ||
       box->source_variable_1!=0 ||
       !world_source::source_player_overlaps_world_sprite(
           *box,actor,box->origin_x,box->origin_y))return false;
    const auto* category=source_box_category(box->source_variable_0);
    const bool nested=box->source_variable_0==51 || box->source_variable_0==52 || box->source_variable_0==53 || box->source_variable_0==999;
    const bool direct=box->source_variable_0>=5 && box->source_variable_0<=13;
    const bool mixed=source_box_has_mixed_drops(box->source_variable_0);
    const bool special=box->source_variable_0==456789;
    if(!category && !nested && !direct && !mixed && !special)return false;
    // Keep the large native ground buffer on the heap, not ARM11 stack.
    auto next=std::unique_ptr<GroundLootSystem>(
        new(std::nothrow) GroundLootSystem(ground));
    auto random=std::unique_ptr<std::mt19937>(new(std::nothrow) std::mt19937(rng));
    if(!next || !random)return false;
    if(nested && !source_box_default_nested_drops(box->source_variable_0,
        *random,*next,box->x,box->y))return false;
    if(direct && !source_box_direct_quantity_drops(box->source_variable_0,
        *next,box->x,box->y))return false;
    if(special && !source_box_456789_drops(*random,*next,box->x,box->y))return false;
    if(mixed && !source_box_mixed_drops(box->source_variable_0,*next,box->x,box->y))return false;
    if(category)for(std::size_t i=0;i<category->count;++i){
        const auto& drop=kSourceBoxDrops[category->first+i];
        const auto pick=drop.choose?source_initial_loot_choice_index(
            (*random)(),drop.count):0;
        const auto requested=kSourceBoxChoices[drop.first+pick];
        const auto id=source_initial_item_after_default_unlocks(requested);
        if(!id || !source_item_exists(id))return false;
        if(!next->add({id,1,kUseSourceItemCondition},
                      box->x+drop.dx,box->y+drop.dy))return false;
    }
    if(!world.open_source_bunker_lootbox(index))return false;
    ground=*next;
    rng=*random;
    return true;
}
}
