#include <opencv2/opencv.hpp>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>

// Function to convert Uint8Array to cv::Mat
cv::Mat convertUint8ArrayToMat(emscripten::val uint8Array, int width, int height) {
    // Get the array length and data pointer
    size_t length = uint8Array["length"].as<size_t>();
    int channels = 4; // RGBA

    // Create a Mat with the specified dimensions and channels
    cv::Mat outputMat(height, width, CV_8UC(channels));

    // Ensure the array size matches expected matrix size
    if (length != static_cast<size_t>(width * height * channels)) {
        throw std::runtime_error("Array size does not match specified dimensions");
    }

    // Copy data from JavaScript Uint8Array to cv::Mat
    emscripten::val heapView = emscripten::val::global("Uint8Array").new_(
        emscripten::typed_memory_view(length, outputMat.data)
    );

    // Set the Mat data from the Uint8Array
    heapView.call<void>("set", uint8Array);

    return outputMat;
}

// 4. C++のcv::MatからJavaScriptのUint8Arrayに変換
emscripten::val convertMatToUint8Array(const cv::Mat &mat) {
    // Create a Uint8Array with the same size as the Mat
    size_t length = mat.total() * mat.elemSize();
    std::vector<uint8_t> outputData(length);
    std::memcpy(outputData.data(), mat.data, length);

    return emscripten::val::global("Uint8Array").new_(emscripten::val::array(outputData));
}



cv::Mat _cropAndResizeImage(
    const cv::Mat &inputImage,
    int inputWidth, int inputHeight,
    int outputWidth, int outputHeight) {

    // 2. アスペクト比を維持しながらリサイズ
    int scaledWidth, scaledHeight;
    if (inputWidth * outputHeight > inputHeight * outputWidth) {
        scaledWidth = outputWidth;
        scaledHeight = inputHeight * outputWidth / inputWidth;
    } else {
        scaledHeight = outputHeight;
        scaledWidth = inputWidth * outputHeight / inputHeight;
    }

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(scaledWidth, scaledHeight));

    // 3. 中央部分を切り出し
    int x = (scaledWidth - outputWidth) / 2;
    int y = (scaledHeight - outputHeight) / 2;
    cv::Rect roi(x, y, outputWidth, outputHeight);
    cv::Mat croppedImage = resizedImage(roi);

    // 4. C++のcv::MatからJavaScriptのUint8Arrayに変換
    return croppedImage;
}

    emscripten::val js_cropAndResizeImage(
        const emscripten::val &inputUint8ArrayOfRgba,
        int inputWidth, int inputHeight,
        int outputWidth, int outputHeight) {
        cv::Mat inputImage = convertUint8ArrayToMat(inputUint8ArrayOfRgba, inputWidth, inputHeight);
        cv::Mat croppedImage = _cropAndResizeImage(inputImage, inputWidth, inputHeight, outputWidth, outputHeight);
        return convertMatToUint8Array(croppedImage);
    }


EMSCRIPTEN_BINDINGS(my_module)
{
    emscripten::function("cropAndResizeImage", &js_cropAndResizeImage);
}

int main() {
    EM_ASM({
        console.log("fs init");
        console.log("fs init done");
        console.log(FS.readdir('/images'));
        FS.readdir('/images').forEach(function (file) {
            console.log(file);
        });
        const uint8Array = FS.readFile('/images/bay.jpg', { encoding: 'binary' });

        // image.src = URL.createObjectURL(new Blob([uint8Array]));
        // JPEG画像をデコードするために、ブラウザのImageオブジェクトを使用する
        const blob = new Blob([uint8Array], { type: 'image/jpeg' });
        const image = await createImageBitmap(blob);
        // document.body.appendChild(image);
        const canvas = document.createElement('canvas');
        const inputWidth = 2000;
        const inputHeight = 1123;
        canvas.width = imageWidth;
        canvas.height = imageHeight;
        const ctx = canvas.getContext('2d');
        ctx.drawImage(image, 0, 0);
        const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        console.log('ImageData:', imageData);
        const rgbaData = imageData.data;
        console.log('RGBA Data:', rgbaData);
        const outputWidth = 720;
        const outputHeight = 1280;
        const croppedImage = Module.cropAndResizeImage(rgbaData, inputWidth, inputHeight, outputWidth, outputHeight);
    });
}
