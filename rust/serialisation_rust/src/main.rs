#[allow(unused)]

extern crate flatbuffers;

mod erreur_generated;


//Import schéma d'erreur
use flatbuffers::FlatBufferBuilder;
use erreur_generated::erreurs::{Erreur, root_as_erreur, ErreurArgs, finish_erreur_buffer};

//Lecture & Ecriture
use std::env;
use std::io; 
use std::fs::File;
use std::io::Write;
use std::io::prelude::*;


//Fonction qui déserialise une erreur
pub fn deserialiser_erreur(buf: &[u8]) -> (i16, &str){
    let erreur = root_as_erreur(buf).unwrap();
    let code = erreur.code();
    let contexte = erreur.contexte().unwrap();
    return (code, contexte);
}

fn main() {
    let mut bldr = FlatBufferBuilder::new();
    let args: Vec<String> = env::args().collect();

    //Récupération des variables de la ligne de commande
    let nom_fichier = &args[1];
    let num = &args[2];
    let code:i16 = num.parse().unwrap();
    let contexte = &args[3];

    //Initialisation du builder
    bldr.reset();

    //Création de l'objet Erreur
    let args = ErreurArgs{
        code: code,
        contexte: Some(bldr.create_string(contexte)),
    };

    let erreur_offset = Erreur::create(&mut bldr, &args);

    //Sérialisation de l'erreur
    finish_erreur_buffer(&mut bldr, erreur_offset);
    let finished_data = bldr.finished_data();


    //Ecriture de la sérialisation dans un fichier.
    let mut file = File::create(nom_fichier).expect("Failed to create or open the file");
    file.write(finished_data);


    //Lecture du fichier
    let mut f = std::fs::File::open(nom_fichier).unwrap();
    let mut buf = Vec::new();
    f.read_to_end(&mut buf).expect("file reading failed");

    //Désérialisation de l'erreur
    let (sortie_code, sortie_contexte) = deserialiser_erreur(&buf[..]);

    println!("Prototype terminé. \ncode = {} \ncontexte = {} \nlongueur de la sérialisation = {}", sortie_code, sortie_contexte, finished_data.len());
}
