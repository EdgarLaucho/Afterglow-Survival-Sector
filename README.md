# Afterglow: Survival Sector

> 🎓 Video Game Project — Master's Degree in Video Game Programming  
> Developed as a team project at **The Core School**

**Afterglow: Survival Sector** is a two-player survival game developed in **Unreal Engine 5** as part of the Master's Degree in Video Game Programming at The Core School.

The project was developed collaboratively, with each team member being responsible for different gameplay systems.

This repository is shared as part of my game development portfolio.

---

## 🎮 About the Game

Afterglow: Survival Sector is a **two-player survival game** where players must fight against waves of enemies and survive increasingly challenging encounters.

The game includes:

- Two-player gameplay
- Ranged combat
- Projectile-based weapons
- Enemy waves
- Enemy AI and combat behavior
- Projectile pooling
- Enemy pooling and spawning
- Survival mechanics

---

# 👨‍💻 My Contributions

My work on the project focused primarily on **gameplay programming using Unreal Engine Blueprints**.

I was responsible for:

- Player implementation
- Shooting mechanics
- Projectile system
- Projectile Object Pool
- Enemy Spawn Pool

Other systems, including the **two-player functionality and enemy behavior/AI**, were developed by other members of the team.

---

## 🧍 Player

I worked on the implementation of the player and its integration with the combat systems.

My work connected the player with the shooting and projectile systems used during gameplay.

---

## 🔫 Shooting System

I designed and implemented the **projectile-based shooting system** using Unreal Engine Blueprints.

The system manages:

- Shooting input
- Projectile activation
- Projectile positioning
- Firing behavior
- Projectile lifecycle
- Integration with the Object Pool system

Instead of creating and destroying a new projectile every time the player fires, projectiles are reused through an Object Pool.

---

## ♻️ Projectile Object Pool

I designed and implemented an **Object Pooling system for projectiles**.

The basic flow is:

1. The player fires.
2. The system requests an available projectile from the pool.
3. The projectile is positioned and activated.
4. The projectile performs its gameplay behavior.
5. When it is no longer required, it is deactivated.
6. The projectile becomes available for reuse.

This avoids repeatedly spawning and destroying projectile actors during gameplay.

---

## 👾 Enemy Spawn Pool

I also implemented the **pooling and spawning system used to manage enemy instances**.

The system handles:

- Maintaining reusable enemy instances
- Retrieving enemies from the pool
- Activating enemies when required
- Positioning them at spawn locations
- Returning enemies to the pool
- Reusing enemy instances during subsequent encounters

This system was designed to support the continuous enemy spawning required by the survival gameplay.

> **Note:** The enemy behavior and AI were developed by another member of the team. My contribution focused specifically on the pooling and spawning architecture.

---

## 👥 Collaborative Development

Afterglow: Survival Sector was developed as a **team project**.

Different members of the team were responsible for different gameplay systems.

My individual programming contributions were:

**Player implementation → Shooting System → Projectile Pool → Enemy Spawn Pool**

The **two-player system and enemy behavior/AI were implemented by another team member.**

This repository therefore contains collaborative work and should not be interpreted as a project developed entirely by me.

---

## 🛠️ Technologies

- **Unreal Engine 5**
- **Blueprints**
- Unreal Engine Gameplay Framework
- Git / Version Control

---

## 🎯 What I Learned

This project gave me practical experience developing gameplay systems using Unreal Engine Blueprints.

One of the main technical areas I explored was **Object Pooling**.

Implementing reusable pools for both projectiles and enemies helped me understand how gameplay objects can be managed efficiently without relying on constant actor creation and destruction.

The project also gave me practical experience with:

- Gameplay programming
- Blueprint architecture
- Object Pooling
- Actor lifecycle management
- Projectile systems
- Enemy spawning
- Player combat
- Blueprint communication
- Debugging
- Team-based development

---

## 📌 Portfolio Notice

This repository is shared for **portfolio and educational purposes**.

Afterglow: Survival Sector was created collaboratively as part of the Master's Degree in Video Game Programming at The Core School.

Not all assets, systems, or gameplay mechanics contained in this repository were created by me.

My personal contributions focused on:

- **Player implementation**
- **Shooting mechanics**
- **Projectile Object Pool**
- **Enemy Spawn Pool**

Other systems, including the two-player functionality and enemy AI/behavior, were developed by other members of the team.
