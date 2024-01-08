use plotters::prelude::*;
use plotters::series::PointSeries;
use rand::rngs::ThreadRng;
use std::error::Error;
use rand::Rng;
use image::{ImageBuffer, Luma, Rgb};

use crate::my_point::*;
use crate::grid::*;

static IMG_W : u32 = 1280;
static IMG_H : u32 = 720;

pub fn height_to_color(z: f32, min_z: f32, max_z: f32) -> RGBColor {
    if z == 0.0 {
        RGBColor((0.0) as u8, (0.0) as u8, (0.0) as u8)
    }
    else {
        let grad = colorgrad::rainbow();

        let normalized_z = (z - min_z) / (max_z - min_z);
        let color = grad.at(normalized_z as f64);
        RGBColor((color.r * 255.0) as u8, (color.g * 255.0) as u8, (color.b * 255.0) as u8)
    }
}

pub fn height_to_grayscale(z: f32, min_z: f32, max_z: f32) -> u8 {
    if z == 0.0 {
        (255.0) as u8
    } else {
        let normalized_z = (z - min_z) / (max_z - min_z);
        (normalized_z * 255.0) as u8
    }
}

pub fn random_color(rng: &mut ThreadRng) -> RGBColor {
    RGBColor(rng.gen(), rng.gen(), rng.gen())
}


pub fn draw_height_cloud(verts: Vec<(f32, f32)>, file_name: &str,  verts_col: RGBColor, points: &Vec<myPoint>, chart_buffer: f32) -> Result<(), Box<dyn Error>> {
    let min_x = verts.iter().map(|(x, _)| x).cloned().fold(f32::INFINITY, f32::min) - chart_buffer;
    let max_x = verts.iter().map(|(x, _)| x).cloned().fold(f32::NEG_INFINITY, f32::max) + chart_buffer;
    let min_y = verts.iter().map(|(_, y)| y).cloned().fold(f32::INFINITY, f32::min) - chart_buffer;
    let max_y = verts.iter().map(|(_, y)| y).cloned().fold(f32::NEG_INFINITY, f32::max) + chart_buffer;

    let file_path = format!("output/{}.png", file_name);
    let root = BitMapBackend::new(&file_path, (IMG_W, IMG_H)).into_drawing_area();
    root.fill(&WHITE)?;

    let mut chart = ChartBuilder::on(&root)
        .caption("Output View", ("Arial", 24).into_font())
        .margin(10)
        .x_label_area_size(20)
        .y_label_area_size(20)
        .build_cartesian_2d(min_x..max_x, min_y..max_y)?;

    chart.configure_mesh().draw();

    chart.draw_series(LineSeries::new(
        verts,
        verts_col.filled(),
    ))?;

    if points.len() > 0 {
        let max_z = points.iter().max_by(|a, b| a.z.partial_cmp(&b.z).unwrap()).unwrap().z;
        let min_z = points.iter().min_by(|a, b| a.z.partial_cmp(&b.z).unwrap()).unwrap().z;
    
        chart.draw_series(points.iter().map(|p| {let color = height_to_color(p.z, min_z, max_z);
            Circle::new((p.x, p.y), 1, color.filled())}));
    }
    else {
        println!("No points to visualize");
    }

    Ok(())
}

pub fn draw_height_cloud_grid(file_name: &str,  grid: &Grid, chart_buffer: f32) -> Result<(), Box<dyn Error>> {
    let min_x = grid.get_min_x() - chart_buffer;
    let max_x = grid.get_max_x() + chart_buffer;
    let min_y = grid.get_min_y() - chart_buffer;
    let max_y = grid.get_max_y() + chart_buffer;

    let file_path = format!("output/{}.png", file_name);
    let root = BitMapBackend::new(&file_path, (IMG_W, IMG_H)).into_drawing_area();
    root.fill(&WHITE)?;

    let mut chart = ChartBuilder::on(&root)
        .caption("Output View", ("Arial", 24).into_font())
        .margin(10)
        .x_label_area_size(20)
        .y_label_area_size(20)
        .build_cartesian_2d(min_x..max_x, min_y..max_y)?;

    chart.configure_mesh().draw();

    let points = grid.get_points();

    if points.len() > 0 {
        let max_z = points.iter().max_by(|a, b| a.z.partial_cmp(&b.z).unwrap()).unwrap().z;
        let min_z = points.iter().filter(|p| p.z != 0.0).min_by(|a, b| a.z.partial_cmp(&b.z).unwrap()).unwrap().z;
        
        chart.draw_series(points.iter().map(|p| {
            let color = height_to_color(p.z, min_z, max_z);
            Circle::new((p.x, p.y), 1, color.filled())}));
    }
    else {
        println!("No points to visualize");
    }

    Ok(())
}

pub fn draw_clusters(verts: Vec<(f32, f32)>, file_name: &str, verts_col: RGBColor, clusters: &Vec<Vec<myPoint>>, chart_buffer: f32) -> Result<(), Box<dyn Error>> {
    let min_x = verts.iter().map(|(x, _)| x).cloned().fold(f32::INFINITY, f32::min) - chart_buffer;
    let max_x = verts.iter().map(|(x, _)| x).cloned().fold(f32::NEG_INFINITY, f32::max) + chart_buffer;
    let min_y = verts.iter().map(|(_, y)| y).cloned().fold(f32::INFINITY, f32::min) - chart_buffer;
    let max_y = verts.iter().map(|(_, y)| y).cloned().fold(f32::NEG_INFINITY, f32::max) + chart_buffer;

    let file_path = format!("output/{}.png", file_name);
    let root = BitMapBackend::new(&file_path, (IMG_W, IMG_H)).into_drawing_area();
    root.fill(&WHITE)?;

    let mut chart = ChartBuilder::on(&root)
        .caption("Output View", ("Arial", 24).into_font())
        .margin(10)
        .x_label_area_size(20)
        .y_label_area_size(20)
        .build_cartesian_2d(min_x..max_x, min_y..max_y)?;

    chart.configure_mesh().draw();

    chart.draw_series(LineSeries::new(
        verts,
        verts_col.filled(),
    ))?;

    let mut rng = rand::thread_rng();

    if clusters.len() > 0 {
        for c in clusters {
            let color = random_color(&mut rng);
            chart.draw_series(c.iter().map(|p| Circle::new((p.x, p.y), 1, color.filled())));
        }
    }
    else {
        println!("No points to visualize");
    }

    Ok(())
}

pub fn create_height_grayscale_image(file_name: &str, mat: &Vec<Vec<f32>>) {
    let width = mat.len() as u32;
    let height = mat[0].len() as u32;

    let mut max_z: f32 = std::f32::MIN;
    let mut min_z: f32 = std::f32::MAX;

    for row in mat {
        for &value in row {
            if value != 0.0 && value < min_z {
                min_z = value;
            }
            if value > max_z {
                max_z = value;
            }
        }
    }

    let mut imgbuf = ImageBuffer::new(width, height);

    for (x, y, pixel) in imgbuf.enumerate_pixels_mut() {
        let val = mat[x as usize][(height - 1 - y) as usize];
        let grey_v = height_to_grayscale(val, min_z, max_z);
        *pixel = Rgb([grey_v, grey_v, grey_v]);
    }

    let file_path = format!("output/{}.png", file_name);
    imgbuf.save(file_path).unwrap();
}

pub fn create_height_rgb_image(file_name: &str, mat: &Vec<Vec<f32>>) {
    let width = mat.len() as u32;
    let height = mat[0].len() as u32;

    let mut max_z: f32 = std::f32::MIN;
    let mut min_z: f32 = std::f32::MAX;

    for row in mat {
        for &value in row {
            if value != 0.0 && value < min_z {
                min_z = value;
            }
            if value > max_z {
                max_z = value;
            }
        }
    }
    
    let mut imgbuf = ImageBuffer::new(width, height);

    for (x, y, pixel) in imgbuf.enumerate_pixels_mut() {
        let val = mat[x as usize][(height - 1 - y) as usize];
        let rgb_value = height_to_color(val, min_z, max_z);
        let (r, g, b) = rgb_value.rgb();
        *pixel = Rgb([r, g, b]);
    }

    let file_path = format!("output/{}.png", file_name);
    imgbuf.save(file_path).unwrap();
}

pub fn create_gradient_grayscale_image(file_name: &str, grad: &Vec<Vec<f32>>) {
    let width = grad.len() as u32;
    let height = grad[0].len() as u32;

    let max_g: f32 = grad.iter().flatten().map(|&g| g.abs()).fold(f32::MIN, f32::max);

    let mut imgbuf = ImageBuffer::new(width, height);

    for (x, y, pixel) in imgbuf.enumerate_pixels_mut() {
        let grad_val = grad[x as usize][(height - 1 - y) as usize].abs();
        let grey_v = (grad_val / max_g * 255.0) as u8;
        *pixel = Rgb([grey_v, grey_v, grey_v]);
    }

    let file_path = format!("output/{}.png", file_name);
    imgbuf.save(file_path).unwrap();
}

pub fn create_gradient_grayscale_image_l(file_name: &str, grad: &Vec<Vec<f32>>, lines: &Vec<Line>) {
    let width = grad.len() as u32;
    let height = grad[0].len() as u32;

    let max_g: f32 = grad.iter().flatten().map(|&g| g.abs()).fold(f32::MIN, f32::max);

    let mut imgbuf = ImageBuffer::new(width, height);

    for (x, y, pixel) in imgbuf.enumerate_pixels_mut() {
        let grad_val = grad[x as usize][(height - 1 - y) as usize].abs();
        let grey_v = (grad_val / max_g * 255.0) as u8;
        *pixel = Rgb([grey_v, grey_v, grey_v]);
    }

    for line in lines {
        let rho = line.rho as u32;
        let theta = line.theta;
        for x in 0..width {
            let y = (rho as f32 - (x as f32 * theta.cos())) / theta.sin();
            if y >= 0.0 && y < height as f32 {
                let pixel = imgbuf.get_pixel_mut(x, y as u32);
                *pixel = Rgb([0, 255, 0]);
            }
        }
    }

    let file_path = format!("output/{}.png", file_name);
    imgbuf.save(file_path).unwrap();
}
