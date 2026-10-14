#include "mmx4_coop_views.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(value) do { if(!(value)) { \
    fprintf(stderr,"line %d: %s\n",__LINE__,#value);exit(1); } } while(0)

int main(void) {
    /* All of P1 must precede all of P2 in the same native eight-bucket
     * layer, including bike4/exhaust6 and wind0. Keep each rider's order. */
    for(unsigned layer=0;layer<4;++layer)for(unsigned a=0;a<=6;a+=2) {
        uint8_t native=(uint8_t)(layer*16+a);
        uint8_t first=mmx4_coop_ride_draw_priority(native,1);
        uint8_t second=mmx4_coop_ride_draw_priority(native,2);
        CHECK((first>>4)==layer && (second>>4)==layer);
        CHECK((first&15)<8 && (second&15)<8);
        for(unsigned b=0;b<=6;b+=2) {
            CHECK(first<mmx4_coop_ride_draw_priority((uint8_t)(layer*16+b),2));
            if(a<b)for(unsigned owner=1;owner<=2;++owner)
                CHECK(mmx4_coop_ride_draw_priority(native,owner)<
                      mmx4_coop_ride_draw_priority((uint8_t)(layer*16+b),owner));
        }
        CHECK(mmx4_coop_ride_draw_priority(native,0)==native);
        CHECK(mmx4_coop_ride_draw_priority(native,3)==native);
    }
    CHECK(mmx4_coop_ride_draw_priority(7,2)==7);
    CHECK(mmx4_coop_ride_draw_priority(0x1A,2)==0x1A);
    /* Native Mushroom lift: limit moves to720 while camera is2047 and the
     * riders are2200/2199. Destination alone wrongly kills both riders. */
    CHECK(2200-8<mmx4_coop_view_death_bottom(2047,720));
    CHECK(2199-8<mmx4_coop_view_death_bottom(2047,720));
    /* Once the lift reaches its destination, falling below it is fatal. */
    CHECK(984-8>=mmx4_coop_view_death_bottom(720,720));
    /* Preserve the earlier Volcano drop: camera still trails above arena. */
    CHECK(650-8<mmx4_coop_view_death_bottom(256,544));
    CHECK(808-8>=mmx4_coop_view_death_bottom(256,544));
    CHECK(!mmx4_coop_views_requested(0,"split"));
    CHECK(!mmx4_coop_views_requested(1,"unified"));
    CHECK(!mmx4_coop_views_requested(1,NULL));
    CHECK(!mmx4_coop_views_requested(1,"invalid"));
    CHECK(mmx4_coop_views_requested(1,"split"));
    CHECK(mmx4_coop_camera_world_ready(6,0,1,-1));
    CHECK(mmx4_coop_camera_world_ready(6,0,2,-1)); /* Partner can survive a P1 death. */
    CHECK(mmx4_coop_camera_world_ready(6,0,3,-1));
    CHECK(!mmx4_coop_camera_world_ready(5,0,1,-1)); /* Checkpoint loader, old bodies. */
    CHECK(!mmx4_coop_camera_world_ready(4,0,1,-1)); /* Full-stage loader. */
    CHECK(!mmx4_coop_camera_world_ready(6,0,0,-1)); /* Mode committed before PLAYER init. */
    CHECK(!mmx4_coop_camera_world_ready(6,1,3,-1)); /* Ordinary death termination. */
    CHECK(!mmx4_coop_camera_world_ready(6,2,1,-1)); /* Native pause. */
    for(int owner=0;owner<=1;++owner) {
        CHECK(mmx4_coop_camera_world_ready(6,1,3,owner)); /* Shared exit fade. */
        CHECK(!mmx4_coop_camera_world_ready(4,0,3,owner));
        CHECK(!mmx4_coop_camera_world_ready(5,0,3,owner));
        CHECK(!mmx4_coop_camera_world_ready(6,2,3,owner));
        CHECK(!mmx4_coop_camera_world_ready(6,1,0,owner));
    }
    CHECK(!mmx4_coop_camera_scope_requested(0,1,1,0,-1));
    CHECK(mmx4_coop_camera_scope_requested(1,1,1,0,-1));
    CHECK(mmx4_coop_camera_scope_requested(0,1,0,1,-1));
    CHECK(mmx4_coop_camera_scope_requested(1,1,0,1,-1));
    CHECK(!mmx4_coop_camera_scope_requested(1,1,0,0,-1));
    CHECK(!mmx4_coop_camera_scope_requested(1,0,1,1,-1));
    /* Jungle's finished P2 exit must not fall back to clamping a hidden P1
     * against the exit's distant, higher room. Retain either scene owner. */
    for(int split=0;split<=1;++split)for(int owner=0;owner<=1;++owner) {
        CHECK(mmx4_coop_camera_scope_requested(split,1,0,0,owner));
        CHECK(!mmx4_coop_camera_scope_requested(split,0,0,0,owner));
    }
    Mmx4CoopViewBounds bounds={0,8192,0,4096,128,192,96,160};
    Mmx4CoopView previous={0,0};
    Mmx4CoopViewActor actors[2]={{56,138,1},{1000,2000,1}};
    Mmx4CoopView views[2]={mmx4_coop_view_follow(previous,actors[0],bounds),
        mmx4_coop_view_follow(previous,actors[1],bounds)};
    CHECK(views[0].camera_x==0 && views[0].camera_y==0);
    CHECK(views[1].camera_x==808 && views[1].camera_y==1840);
    Mmx4CoopView still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);
    actors[0].world_x=3000;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);
    actors[1].world_x=-50;actors[1].world_y=5000;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==0 && still.camera_y==4096);
    actors[1].active=0;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);

    PSXPlacementRect regions[2];
    actors[1].active=1;
    unsigned count=mmx4_coop_view_regions(views,actors,-1,0,regions);
    CHECK(count==2);
    CHECK(mmx4_coop_view_contains(regions,count,80,140));
    CHECK(mmx4_coop_view_contains(regions,count,1000,2000));
    CHECK(!mmx4_coop_view_beam_contains(views[0],1000,2000,0,64,0,64,0));
    CHECK(mmx4_coop_view_beam_contains(views[1],1000,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[0],100,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[1],100,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[0],600,1000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[1],600,1000,0,64,0,64,0));
    CHECK(mmx4_coop_view_beam_contains(views[0],400,140,-240,-220,0,16,0));
    CHECK(!mmx4_coop_view_contains(regions,count,500,1000));
    CHECK(!mmx4_coop_view_contains(regions,count,80,2000));
    CHECK(!mmx4_coop_view_contains(regions,count,1000,140));
    CHECK(!mmx4_coop_view_contains(regions,count,-48,140));
    CHECK(!mmx4_coop_view_contains(regions,count,368,140));
    CHECK(mmx4_coop_view_regions(views,actors,1,0,regions)==1);
    CHECK(!mmx4_coop_view_contains(regions,1,80,140));
    CHECK(mmx4_coop_view_contains(regions,1,1000,2000));
    actors[1].active=0;
    CHECK(mmx4_coop_view_regions(views,actors,-1,0,regions)==1);
    actors[0].active=0;
    CHECK(!mmx4_coop_view_regions(views,actors,-1,0,regions));

    actors[0]=(Mmx4CoopViewActor){0,0,1};
    actors[1]=(Mmx4CoopViewActor){1000,0,1};
    CHECK(mmx4_coop_view_nearest(actors,900,0,0)==1);
    CHECK(mmx4_coop_view_nearest(actors,100,0,1)==0);
    CHECK(mmx4_coop_view_nearest(actors,500,0,0)==0);
    CHECK(mmx4_coop_view_nearest(actors,500,0,1)==1);
    actors[0].active=0;
    CHECK(mmx4_coop_view_nearest(actors,0,0,0)==1);

    Mmx4CoopViewVisits visits={0};
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,0);
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,1);
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,1));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,0);
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,0,0);
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,0,0));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,MMX4_COOP_PLACEMENTS,1,0));
    puts("mmx4_coop_views_test: PASS");
    return 0;
}
