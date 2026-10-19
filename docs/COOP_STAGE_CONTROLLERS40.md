# Playtest39 stage-controller follow-up

The five qualified fixes (183,184,190,201,202) are selected for Playtest40,
alongside the personal-menu/private-energy work based on commit `948f6a25`.
The burndown tracks each of the21 incoming symptoms separately (182–202).
Unqualified Dragoon and lift-command candidates are excluded from the release;
their private patch is retained under `build-pr23/feedback39/deferred-magma-lift.patch`.

## Supplied evidence

The eight diagnostic ZIPs cover Storm Owl lasers, Mushroom latches/midboss,
Mushroom lift, Chaser dash, Slash Beast, Dragoon and two opening-stage runs.
The indexed sessions identify Playtest39 offline local split, Both doors and
boss scaling. Cumulative archives are deduplicated by session, keeping the
longest input stream. Originals remain unchanged.

Videos indexed: O7cQUeDZGkc, OA-BsVELu7Q, GbEFAAiIEIY, KGr_STHBYsE,
9_7YxBHgcT0, PFuk8tsshi0 and rFtnY31SJcU. Contact sheets help locate events;
they are not a claim that every video frame was reviewed.

## Native qualifications so far

All runs use the original game executable/disc, hidden copies on a private
Windows desktop, dummy audio, and local8:3 Split. Stage/checkpoint, position,
invulnerability and enemy-health fixtures prepare scenarios. Native controller
input and original actors commit the events below. These are controlled native
scenarios, not exact replays of the tester's input stream.

| Item | Observation and limit |
| --- | --- |
| 183 | Original39 closes the hatch while P2 is below it. Revised logic keeps state1 with the separated players and reaches state2 after both clear the collision edge. Both retainHP32. The first close follow-up used an invalid staggered setup; the final test moves both at one native boundary. |
| 184 | Before revision, a P2 entry around4752 began the pan from the distant P1 camera near5192. Revised entry begins around4608/4592 in P2's room. A subsequent native slash supplies the final hit after an enemy-health setup; the original death sequence retires the actor with both heroes healthy. This does not establish the separate post-fight traversal report187. |
| 190 | Original39 waits in its initial train phase despite P2 crossing the first threshold. Revised controller admits and finishes native bomb scenes at2256,3376 and6736. P1 is alive for the first two and dead for the last; P2 remains healthy. An initial test incorrectly expected a bomb admission at5056: native D8A64 is a no-op there. That timeout is not evidence of a survivor bug. |
| 202 | With P1 alive at1616/443, P2 crosses the final slab and descends to954. The original closing-wall actors advance from state1 to state2, and both retainHP32. Only the predicate's loaded Y changes; native movement, map edits and collision still run once. |
| 201 | Matched movement reproduces the difference: P1 is airborne in action7 atY473.94/689.29/845.29 while P2 remains grounded in action4/5/2 atY496/722/878. Native BED90 writes-1 to the slab's carry field before collision. P2's cached positive carry replaced it. The final native retest keeps both airborne in action7 atY463.11/462.11,598.29/597.29,741.29/740.29 and916.79/915.79. They land at955/954 withHP32 each. A boundary regression also verifies that the override is temporary and does not disable later normal carrying. |

## Candidates and remaining checks

- The deferred lift camera candidate treats+47 as a command written every tick, including
  repeated identical values. Its regression check passes. Short native ascent,
  jump and P1-death controls preserve P2 health; they do not yet qualify the
  reported high-jump offset or blue-panel mechanics.
- The deferred Dragoon candidate selects the progressing/nearby living hero. A newly
  allocated falling fireball has no coordinates and initializes relative to
  CAMERA; it must retain the allocating seat and that seat's camera rather than
  choose the nearest hero to0/0. Bounded before/after field qualification remains
  required. Do not confuse authored rolling fireballs with the random falling
  branch.
- An original39 control already spawns some part2 platform fireballs, including
  an832 source, with P1 behind. This bounded result does not reproduce item195;
  do not claim that report is fixed merely because the candidate also spawns
  them. The sampled part1 location1425 precedes the next controller's placement
  window and does not qualify the random falling branch.
- The Slash midboss actor spawns in a bounded P2-led survivor setup. Placement
  alone does not establish fight behavior. Part2 stop/start probes initially
  teleported into unsafe geometry and are excluded; those reports remain open.
- The initial opening-stage spawn check found static type7 wall enemies. The
  later flying groups are allocated by BF3B0 after falling-object phases, so
  that check does not clear item200. A walk blocked before a trigger threshold
  does not establish an object-trigger failure either.
- Long teleports can leave the eased camera/placement window behind the hero.
  Wait on safe terrain before movement. Initial pit/crush deaths from invalid
  setup positions are excluded from the reported regressions.
- Storm laser projection, remaining Mushroom traversal/panels, exact Chaser
  dash recurrence, Slash background/stop/start, Dragoon endpoints/shake and the
  remaining opening effects/spawns stay open. No LAN qualification is claimed
  for this batch yet.

## Engineering checks and private artifacts

The Windows runtime builds and all36 CTest checks pass. GCC/Linux lifecycle,
combat and split boundary executables also compile and pass, including the
negative carry override. These model checks supplement native qualifications;
they do not prove a stage mechanic by themselves.

Private evidence is under `build-pr23/feedback39`: intake manifests and unique
sessions; original39 surveys; `candidate-refined`; `candidate-focused`;
`candidate-controls`; `midboss-native-kill`; `candidate-final`; and the
`lava-original39` control. JSON observations include
native body mirrors, saved camera state, controller/actor bytes and original
enemy state. `*-pair.png` captures the composed8:3 presentation, while plain
captures show only the canonical view. Survey wrappers retain per-case errors;
their outer harness status must not be treated as an all-green result.

Tracking: `beads-t56p` and `beads-eio.9.197` in the central database.
