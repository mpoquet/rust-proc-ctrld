extern crate flatbuffers;

#[allow(dead_code, unused_imports)]
#[path = "./monster_generated.rs"]
mod monster_generated;
pub use monster_generated::my_game::sample::{root_as_monster,
                                            Color, Equipement,
                                            Monster, MonsterArgs,
                                            Vec3,
                                            Weapon, WeaponArgs};


fn main() -> std::io::Result<()> {
    let mut builder = flatbuffers::FlatBufferBuilder::with_capacity(1024);

    let weapon_one_name = builder.create_string("Sword");
    let weapon_two_name = builder.create_string("Axe");

    let sword = Weapon::create(&mut builder, &WeaponArgs{
        name: Some(weapon_one_name),
        damage: 3,
    });
    let axe = Weapon::create(&mut builder, &WeaponArgs{
        name: Some(weapon_two_name),
        damage: 5,
    });

    let weapons = builder.create_vector(&[sword, axe]);

    let inventory = builder.create_vector(&[0u8, 1, 2, 3, 4, 5, 6, 7, 8, 9]);

    let x = Vec3::new(1.0, 2.0, 3.0);
    let y = Vec3::new(4.0, 5.0, 6.0);
    let path = builder.create_vector(&[x, y]);

    let name = builder.create_string("Orc");

    let orc = Monster::create(&mut builder, &MonsterArgs{
        pos: Some(&Vec3::new(1.0f32, 2.0f32, 3.0f32)),
        mana: 150,
        hp: 80,
        name: Some(name),
        inventory: Some(inventory),
        color: Color::Red,
        weapons: Some(weapons),
        equipped_type: Equipement::Weapon,
        equipped: Some(axe.as_union_value()),
        path: Some(path),
        ..Default::default()
    });

    builder.finish(orc, None);

    let buf = builder.finished_data();
    std::fs::write("buffer.fb", buf)?; //create and write our buffer in a FlatBuffers binary file (fast to access and compact)
    Ok(())
}