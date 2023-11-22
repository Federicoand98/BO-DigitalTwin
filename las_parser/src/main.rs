#![allow(dead_code, unused_variables)]

pub mod las_wrapper;
pub mod dataframe_wrapper;
pub mod program;

use std::error::Error;
use program::Program;

fn main() -> Result<(), Box<dyn Error>> {

    let mut program = Program::new(
        "assets/raw_point_cloud/",
        "assets/filtered_point_cloud/",
        "assets/filtered_buildings_tiles.csv"
    );

    program.run();

    Ok(())
}