use std::{io::BufWriter, fs::File, error::Error};

use las::{Read, Reader, Write, Writer, Point, Color, Header, reader::PointIterator};

pub struct LasReader {
    reader: Reader<'static>,
}

pub struct LasWriter {
    writer: Writer<BufWriter<File>>
}

impl LasReader {

    pub fn new(file_path: &str) -> Result<Self, Box<dyn Error>>{
        let reader = Reader::from_path(file_path).expect("Failed to read file");

        Ok(Self { reader })
    }

    pub fn get_header(&self) -> Result<Header, Box<dyn Error>> {
        Ok(self.reader.header().clone())
    }

    pub fn get_points(&mut self) -> Result<PointIterator<'_>, Box<dyn Error>> {
        let points = self.reader.points();
        Ok(points)
    }
}

impl LasWriter {

    pub fn new(file_path: &str, header: Header) -> Result<Self, Box<dyn Error>> {
        let writer = Writer::from_path(file_path, header).expect("Failed to create file");
        Ok(Self { writer })
    }

    pub fn write_points(&mut self, points: Vec<Point>) -> Result<(), Box<dyn Error>> {
        for point in points {
            self.writer.write(point).expect("Failed to write a point");
        }

        Ok(())
    }

    pub fn close(&mut self) {
        self.writer.close();
    }
}