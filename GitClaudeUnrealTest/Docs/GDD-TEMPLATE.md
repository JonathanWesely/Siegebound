# <Game Title> — Game Design Document

<!-- Copy this structure into GDD.md. Every section is optional except Overview and Mechanics, but the more precise you are, the better the manager's task breakdown. Concrete numbers beat adjectives: "sprint = 600 units/s" is buildable; "fast movement" is a guess. -->

## 1. Overview
Genre, platform, camera (e.g. third-person), one-paragraph pitch.

## 2. Core Gameplay Loop
The 30-second loop the player repeats. Example: explore → mine ore → return to base → upgrade pickaxe → unlock deeper caves.

## 3. Mechanics
One subsection per mechanic. Include acceptance criteria — how you'd test it works.

### 3.1 <Mechanic name>
- What the player does, what happens, edge cases
- Acceptance: "clicking a rock 3 times breaks it and spawns 1–3 ore pickups"

## 4. Characters & Enemies
Player abilities/stats; each enemy type: behavior, health, damage, where it spawns.

## 5. Levels / World
Map list, layout descriptions, what's in each area, win/lose conditions.

## 6. Art Style & Audio
Visual references, color palette, mood, low-poly vs realistic, sound direction. This drives the art-director's choices.

## 7. UI / UX
Screens (menu, HUD, inventory, pause), what each shows, and when.

## 8. Progression & Economy
Currencies, upgrades, costs, unlock order.

## 9. Milestones
Ordered playable increments. The manager uses this section directly if present; otherwise it proposes its own. Milestone 1 should be the smallest thing you can walk around in and test.

1. <e.g. Player can move, mine one rock type, see ore count in HUD>
2. <e.g. Base + upgrade shop + second cave area>

## 10. Out of Scope
What NOT to build yet — keeps agents from gold-plating.
