#![allow(dead_code, unused_variables)]

use std::error::Error;

use las::{Read, Reader, Write, Writer };
use polars::prelude::*;
use polars_core::prelude::*;
use polars_io::prelude::*;
use geo::{Point, Polygon, Coordinate};
use geo::algorithm::contains::Contains;

fn main() -> Result<(), Box<dyn Error>> {
    let mut df = read_csv().unwrap();

    let min_x: f64 = 684000.0;
    let min_y: f64 = 4930000.0;

    let geo_points_utm = df.column("Geo Point UTM")?.clone();
    let geo_points_utm = geo_points_utm.utf8().unwrap();
    let geo_points_utm: Vec<Vec<&str>>= geo_points_utm
        .into_iter()
        .map(|s| s.unwrap().trim_matches(|c| c == '[' || c == ']').split(", ").collect())
        .collect();

    let x: Vec<f64> = geo_points_utm.iter().map(|v| v[1].parse().unwrap()).collect();
    let y: Vec<f64> = geo_points_utm.iter().map(|v| v[2].parse().unwrap()).collect();

    df = df.with_column(Series::new("X", x)).unwrap().clone();
    df = df.with_column(Series::new("Y", y)).unwrap().clone();

    let range_x = (min_x - 25.0, min_x + 525.0);
    let range_y = (min_y - 25.0, min_y + 525.0);

    let mask_x = df.column("X")?.f64().unwrap().gt(range_x.0) & df.column("X")?.f64().unwrap().lt(range_x.1);
    let mask_y = df.column("Y")?.f64().unwrap().gt(range_y.0) & df.column("Y")?.f64().unwrap().lt(range_y.1);
    let mask = mask_x & mask_y;

    let df_filtered = df.filter(&mask).unwrap();

    let polygons = build_polygons(&df_filtered);

    let z: Vec<f64> = df_filtered.column("QUOTA_GRONDA")?.clone().f64().unwrap().into_iter().map(|v| v.unwrap()).collect();


    set_z_las(polygons?, z);

    Ok(())
}

fn build_polygons(dataframe: &DataFrame) -> Result<Vec<Polygon<f64>>, Box<dyn Error>> {
    let geo_shapes = dataframe.column("Geo Shape UTM")?.clone();
    let geo_shapes = geo_shapes.utf8().unwrap();
    let geo_shapes: Vec<Vec<Vec<&str>>> = geo_shapes
        .into_iter()
        .map(|s| s.unwrap().trim_matches(|c| c == '[' || c == ']').split("], [").map(|s| s.split(", ").collect()).collect())
        .collect();

    let mut polygons: Vec<Polygon<f64>> = Vec::new();

    for shape in geo_shapes {
        let mut points: Vec<Coordinate<f64>> = Vec::new();

        for point in shape {
            let x: f64 = point[1].parse().unwrap();
            let y: f64 = point[2].parse().unwrap();

            points.push(Coordinate { x, y });
        }

        let polygon = Polygon::new(points.into(), vec![]);
        polygons.push(polygon);
    }

    Ok(polygons)
}

fn read_csv() -> PolarsResult<DataFrame> {
    CsvReader::from_path("assets/filtered_buildings.csv")?
            .has_header(true)
            .with_separator(b';')
            .finish()
}

fn set_z_las(polygons: Vec<Polygon<f64>>, z: Vec<f64>) {
    let start = std::time::Instant::now();

    println!("Z: {:?}", z.len());

    let mut reader = Reader::from_path("assets/32_684000_4930000.las").expect("Failed to read file");
    let mut writer = Writer::from_path("assets/new_las.las", reader.header().clone()).expect("Failed to create file");

    for wrapped_point in reader.points() {
        let mut found = false;
        let mut point = wrapped_point.unwrap();
        point.nir = Some(0);

        for (i, polygon) in polygons.iter().enumerate() {
            if polygon.contains(&Point::new(point.x, point.y)) {
                if point.z >= z[i] {
                    found = true;
                    break;
                }
            }
        }

        if !found {
            point.z = 0.0;
        }

        writer.write(point).expect("Failed to write point");
    }

    writer.close().expect("Failed to close writer");

    let duration = start.elapsed();
    println!("Time elapsed in expensive_function() is: {:?}", duration);
}