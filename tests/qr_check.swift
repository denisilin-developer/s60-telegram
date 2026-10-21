import CoreImage
import Foundation

let lines = String(data: FileHandle.standardInput.readDataToEndOfFile(), encoding: .utf8)!
    .split(separator: "\n").map(Array.init)
let n = lines.count, scale = 8, quiet = 4, side = (n + 2 * quiet) * scale
var px = [UInt8](repeating: 255, count: side * side)
for y in 0..<n { for x in 0..<n where lines[y][x] == "1" {
    for dy in 0..<scale { for dx in 0..<scale {
        px[((y + quiet) * scale + dy) * side + (x + quiet) * scale + dx] = 0 } } } }
let img = CIImage(bitmapData: Data(px), bytesPerRow: side, size: CGSize(width: side, height: side),
                  format: .L8, colorSpace: CGColorSpaceCreateDeviceGray())
let det = CIDetector(ofType: CIDetectorTypeQRCode, context: nil, options: nil)!
let msg = (det.features(in: img).first as? CIQRCodeFeature)?.messageString
print(msg ?? "<no QR found>")
