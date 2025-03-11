# Démon de contrôle

## Description

Ce projet a pour but de développer une <b>application distribuée légère et robuste </b> pour
l’exécution et le contrôle des exécutions de processus sur un ensemble de machine. Ce sera dans le
langage <b>Rust</b> et dans le langage <b>C</b>. On y fera des <b>comparaisons entre les langages</b>, afin de répondre
au mieux aux exigences. Nous appellerons cette application un « Démon de Contrôle ». Cette
application utilisera une architecture basée sur des démons, avec un processus longue durée sur
chaque machine, offrant une interface de contrôle distant.

## Structure du projet

```
├── c
│   ├── include
│   │   └── type.h
│   └── src
│       └── main.c
├── LICENSE.Apache-2.0.md
├── LICENSE.CC-BY-4.0.md
├── README.md
└── rust
    ├── Cargo.lock
    ├── Cargo.toml
    └── src
        ├── main.rs
        └── prototype.rs

6 directories, 9 files
```

## Crédit

FORT Alexandre  
MAATI Mohamed  
FIEUX Telmo  
LAGIER Hadrien  

## License
- Code: Apache-2.0
- Everything else, in particular documentation and measurements: CC-BY-SA-4.0
