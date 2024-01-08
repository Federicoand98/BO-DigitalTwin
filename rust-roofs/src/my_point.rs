#[derive(Clone)]
pub struct myPoint {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl myPoint {
    pub fn new (x: f32, y: f32, z: f32) -> Self {
        Self { x:x, y:y, z:z }
    }

    pub fn distance_xy(&self, other: &Self) -> f32 {
        ((other.x - self.x).powi(2) + (other.y - self.y).powi(2)).sqrt()
    }
}

pub struct Line {
    pub rho: f32,
    pub theta: f32,
}