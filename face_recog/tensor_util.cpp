#include "tensor_util.h"

static Status ReadEntireFile(tensorflow::Env* env, const string& filename,
	Tensor* output) {
	tensorflow::uint64 file_size = 0;
	TF_RETURN_IF_ERROR(env->GetFileSize(filename, &file_size));

	string contents;
	contents.resize(file_size);

	std::unique_ptr<tensorflow::RandomAccessFile> file;
	TF_RETURN_IF_ERROR(env->NewRandomAccessFile(filename, &file));

	tensorflow::StringPiece data;
	TF_RETURN_IF_ERROR(file->Read(0, file_size, &data, &(contents)[0]));
	if (data.size() != file_size) {
		return tensorflow::errors::DataLoss("Truncated read of '", filename,
			"' expected ", file_size, " got ",
			data.size());
	}
	output->scalar<string>()() = data.ToString();
	return Status::OK();
}



// Given an image file name, read in the data, try to decode it as an image,
// resize it to the requested size, and then scale the values as desired.
Status ReadTensorFromImageFile(const string& file_name, const int input_height,
	const int input_width, const float input_mean,
	const float input_std,
	std::vector<Tensor>* out_tensors) {
	auto root = tensorflow::Scope::NewRootScope();
	using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

	string input_name = "file_reader";
	string output_name = "normalized";

	// read file_name into a tensor named input
	Tensor input(tensorflow::DT_STRING, tensorflow::TensorShape());
	TF_RETURN_IF_ERROR(
		ReadEntireFile(tensorflow::Env::Default(), file_name, &input));

	// use a placeholder to read input data
	auto file_reader =
		Placeholder(root.WithOpName("input"), tensorflow::DataType::DT_STRING);

	std::vector<std::pair<string, tensorflow::Tensor>> inputs = {
		{ "input", input },
	};

	// Now try to figure out what kind of file it is and decode it.
	const int wanted_channels = 3;
	tensorflow::Output image_reader;
	if (tensorflow::StringPiece(file_name).ends_with(".png")) {
		image_reader = DecodePng(root.WithOpName("png_reader"), file_reader,
			DecodePng::Channels(wanted_channels));
	}
	else if (tensorflow::StringPiece(file_name).ends_with(".gif")) {
		// gif decoder returns 4-D tensor, remove the first dim
		image_reader =
			Squeeze(root.WithOpName("squeeze_first_dim"),
				DecodeGif(root.WithOpName("gif_reader"), file_reader));
	}
	else if (tensorflow::StringPiece(file_name).ends_with(".bmp")) {
		image_reader = DecodeBmp(root.WithOpName("bmp_reader"), file_reader);
	}
	else {
		// Assume if it's neither a PNG nor a GIF then it must be a JPEG.
		image_reader = DecodeJpeg(root.WithOpName("jpeg_reader"), file_reader,
			DecodeJpeg::Channels(wanted_channels));
	}
	// Now cast the image data to float so we can do normal math on it.
	auto float_caster =
		Cast(root.WithOpName("float_caster"), image_reader, tensorflow::DT_FLOAT);
	// The convention for image ops in TensorFlow is that all images are expected
	// to be in batches, so that they're four-dimensional arrays with indices of
	// [batch, height, width, channel]. Because we only have a single image, we
	// have to add a batch dimension of 1 to the start with ExpandDims().
	auto dims_expander = ExpandDims(root, float_caster, 0);
	// Bilinearly resize the image to fit the required dimensions.
	auto resized = ResizeBilinear(
		root, dims_expander,
		Const(root.WithOpName("size"), { input_height, input_width }));
	// Subtract the mean and divide by the scale.
	Div(root.WithOpName(output_name), Sub(root, resized, { input_mean }), { input_std });

	// This runs the GraphDef network definition that we've just constructed, and
	// returns the results in the output tensor.
	tensorflow::GraphDef graph;
	TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

	std::unique_ptr<tensorflow::Session> session(
		tensorflow::NewSession(tensorflow::SessionOptions()));
	TF_RETURN_IF_ERROR(session->Create(graph));
	TF_RETURN_IF_ERROR(session->Run({ inputs }, { output_name }, {}, out_tensors));
	return Status::OK();
}


// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status LoadGraph(const string& graph_file_name,
	std::unique_ptr<tensorflow::Session>* session) {
	tensorflow::GraphDef graph_def;
	Status load_graph_status =
		ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
	if (!load_graph_status.ok()) {
		return tensorflow::errors::NotFound("Failed to load compute graph at '",
			graph_file_name, "'");
	}
	session->reset(tensorflow::NewSession(tensorflow::SessionOptions()));
	Status session_create_status = (*session)->Create(graph_def);
	if (!session_create_status.ok()) {
		return session_create_status;
	}

    // TODO use GPU


	return Status::OK();
}

// Analyzes the output of the Inception graph to retrieve the highest scores and
// their positions in the tensor, which correspond to categories.
Status GetTopLabels(const std::vector<Tensor>& outputs, int how_many_labels,
	Tensor* indices, Tensor* scores) {
	auto root = tensorflow::Scope::NewRootScope();
	using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)

	string output_name = "top_k";
	TopK(root.WithOpName(output_name), outputs[0], how_many_labels);
	// This runs the GraphDef network definition that we've just constructed, and
	// returns the results in the output tensors.
	tensorflow::GraphDef graph;
	TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

	std::unique_ptr<tensorflow::Session> session(
		tensorflow::NewSession(tensorflow::SessionOptions()));
	TF_RETURN_IF_ERROR(session->Create(graph));
	// The TopK node returns two outputs, the scores and their original indices,
	// so we have to append :0 and :1 to specify them both.
	std::vector<Tensor> out_tensors;
	TF_RETURN_IF_ERROR(session->Run({}, { output_name + ":0", output_name + ":1" },
	{}, &out_tensors));
	*scores = out_tensors[0];
	*indices = out_tensors[1];
	return Status::OK();
}

// Given the output of a model run, and the labels
// this prints out the highest-scoring value.
Status PrintFirstLabel(const std::vector<Tensor>& outputs,
	const std::vector<string> labels) {
	Tensor indices;
	Tensor scores;
	TF_RETURN_IF_ERROR(GetTopLabels(outputs, 1, &indices, &scores));
	tensorflow::TTypes<float>::Flat scores_flat = scores.flat<float>();
	tensorflow::TTypes<int32>::Flat indices_flat = indices.flat<int32>();
	const int label_index = indices_flat(0);
	const float score = scores_flat(0);
	LOG(INFO) << labels[label_index] << " (" << label_index << "): " << score;
	return Status::OK();
}


Tensor ReadTensorFromCvMat(cv::Mat image, const int height, const int width) {

	Tensor input_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, height, width, 3 })); // New Tensor shape [1, ndim]
	auto input_tensor_mapped = input_tensor.tensor<float, 4>();

	const float* source_data = (float*)image.data;

	for (int y = 0; y < height; ++y) {
		const float* source_row = source_data + (y * width * 3);
		for (int x = 0; x < width; ++x) {
			const float* source_pixel = source_row + (x * 3);
			input_tensor_mapped(0, y, x, 0) = source_pixel[2];
			input_tensor_mapped(0, y, x, 1) = source_pixel[1];
			input_tensor_mapped(0, y, x, 2) = source_pixel[0];
			//if (y == 0) {
			//	LOG(INFO) << "[" << input_tensor_mapped(0, y, x, 0) << "," << input_tensor_mapped(0, y, x, 1) << "," << input_tensor_mapped(0, y, x, 1) << "]";
			//}
		}
	}

	return input_tensor;
}

