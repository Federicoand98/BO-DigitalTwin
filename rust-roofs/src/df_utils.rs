use geo::{Polygon, Contains, Point, Coord};
use polars::prelude::*;
use std::error::Error;

pub fn get_geo_shapes_off(mut df: DataFrame) -> Result<Vec<Vec<Vec<String>>>, Box<dyn Error>> {
    let geo_shapes = df.column("Geo Shape UTM_OFF")?.clone();
    
    let geo_shapes: Vec<Vec<Vec<String>>> = geo_shapes.utf8().unwrap()
        .into_iter()
        .map(|s| {
            s.unwrap().trim_matches(|c| c == '[' || c == ']')
                .split("], [")
                .map(|s| s.split(", ").map(String::from).collect())
                .collect()
        })
        .collect();

    Ok(geo_shapes)
}

pub fn get_geo_shapes_std(mut df: DataFrame) -> Result<Vec<Vec<Vec<String>>>, Box<dyn Error>> {
    let geo_shapes = df.column("Geo Shape UTM")?.clone();
    
    let geo_shapes: Vec<Vec<Vec<String>>> = geo_shapes.utf8().unwrap()
        .into_iter()
        .map(|s| {
            s.unwrap().trim_matches(|c| c == '[' || c == ']')
                .split("], [")
                .map(|s| s.split(", ").map(String::from).collect())
                .collect()
        })
        .collect();

    Ok(geo_shapes)
}

pub fn get_polygons(mut df: DataFrame, mut off: bool) -> Result<Vec<Polygon<f64>>, Box<dyn Error>> {
    let mut polygons: Vec<Polygon<f64>> = Vec::new();
    let geo_shapes;
    if off {
        geo_shapes = get_geo_shapes_off(df).unwrap();
    }
    else {
        geo_shapes = get_geo_shapes_std(df).unwrap();
    }

    for shape in geo_shapes {
        let mut points: Vec<Coord<f64>> = Vec::new();

        for point in shape {
            let x: f64 = point[0].parse().unwrap();
            let y: f64 = point[1].parse().unwrap();

            points.push(Coord { x, y });
        }

        let polygon = Polygon::new(points.into(), vec![]);
        polygons.push(polygon);
    }

    Ok(polygons)
}
