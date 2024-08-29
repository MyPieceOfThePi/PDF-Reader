#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

struct xref_value {
	int adr;
	int gen;
	char free;
};

//Reads until a delimiter and returns the output string
std::string read_until(std::ifstream& file, std::vector<char> delim) {
	char c = '0';
	std::string out;
	while (std::count(delim.begin(), delim.end(), file.peek()) == 0 && file.peek() != EOF) {
		file.get(c);
		out.push_back(c);
	}
	return out;
}

class PDF {
public:
	
	//Constructs PDF object and opens the file
	PDF(std::string file_path) : pdf(file_path, std::ifstream::binary) {
	}

	//Verifys the file is in the correct format by checking for the %PDF file header
	bool verify() {
		std::string buffer;
		std::getline(pdf, buffer);
		buffer = buffer.substr(0, 4);
		return buffer == "%PDF";
	}

	//Parses a PDF dictionary (WIP)
	void parse_dict(std::ifstream& file) {
		char buf[3]; file.read(buf, 2); buf[2] = '\0';
		std::string start = buf;
		file.ignore();
		if (start == "<<") {
			while (file.peek() != '>' && file.peek() != EOF) {
				std::string key; std::getline(file, key);
				std::string value = read_until(file, { '/' ,'>' });
				file.ignore();
				std::cout << key << " : " << value << std::endl;
			}
		}
		else {
			std::cout << "Expecting << instead found " << start;
		}
	}

	/* Parses the xref(crossreference) table
	The crossreference table stores the adresses for each object in a pdf.
	Each line is stored in the format nnnnnnnnnn ggggg n EOL.

	n is the byte offset of the object
	g is a generation number and should be 00000
	the final n denotes that the object is in use

	Overall the xref table has this format
	xref
	start_index length
	nnnnnnnnnn ggggg n
	nnnnnnnnnn ggggg n
	...
	trailer */
	void parse_xref() {
		//Moves to the beggining of the xref table
		pdf.seekg(xref_adr, pdf.beg);
		std::string buffer; std::getline(pdf, buffer);

		//Checks the table begins with xref
		if (buffer == "xref") {
			//Reads the start_indx and table length and stores them
			int start_indx = stoi(read_until(pdf, { ' ' }));
			pdf.ignore();
			int num_objs = stoi(read_until(pdf, { '\n' }));
			pdf.ignore();

			//Loops through every row in the xref table and stores it
			for (int i = 0; i < num_objs; i++) {
				char buf[21];
				pdf.read(buf, 20);
				buf[20] = '\0';

				std::string row = buf;
				row.erase(std::remove(row.begin(), row.end(), '\n'), row.end());

				xref_value xref_row;
				xref_row.adr = stoi(row.substr(0, 10));
				xref_row.gen = stoi(row.substr(10, 5));
				xref_row.free = row.at(15);
				xref_table.push_back(xref_row);
			}
		}
		//Throws error if keyword xref not found
		else {
			std::cout << "xref table not at expected position";
		}
	}

	/** Finds the xref table adress from the EOF
		the end of file is formatted as

		startxref
		xref_adr
		%%EOF
	**/
	int find_xref() {
		//moves to the end of the second to last line
		//width of the last line is always 8 bytes including EOL chars
		pdf.seekg(-8, pdf.end);

		//moves backwards until it fines an endl character
		char buf = 0;

		while (buf != '\n') {
			pdf.get(buf);
			pdf.seekg(-2, pdf.cur);
		}
		pdf.seekg(2, pdf.cur);

		//gets the address and returns it
		std::string buffer;
		std::getline(pdf, buffer);

		int adr = stoi(buffer);
		xref_adr = adr;
		return adr;
	}

	//Parses the PDF file
	void parse() {
		if (pdf.is_open()) {
			//Checks that the file is a PDF
			if (verify()) {

				//Finds and parses the xref table
				find_xref();
				parse_xref();

				//
				std::string buffer; std::getline(pdf, buffer);
				std::cout << buffer << std::endl;
				parse_dict(pdf);
			}
			//Throws error if file is not pdf
			else {
				std::cout << "File is not PDF";
			}
		}
		//Throws error if file can not be opened
		else {
			std::cout << "Could not open file";

		}
	}

private:
	std::ifstream pdf;
	int xref_adr = -1;
	std::vector<xref_value> xref_table;
};

int main() {
	std::string file_path = "C:/Users/user/Downloads/Untitled presentation.pdf";
	PDF pdf(file_path);
	pdf.parse();
	return 0;
}
