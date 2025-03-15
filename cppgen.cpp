#include <pch.h>
#include "MLGraph.xaml.h"
#include "MLGraph.g.h"


std::string XL::GenerateCode()
{
    std::string r;

    // Initialization
    r += R"(#include "pch.h"
#include "dmllib.hpp"

int main()
{   
    // Initialize DirectML via the DMLLib
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ML ml(true);
	ml.SetFeatureLevel(DML_FEATURE_LEVEL_6_4);
	auto hr = ml.On();
	if (FAILED(hr))
		return 0;


)";

    for (size_t iop = 0; iop < ops.size(); iop++)
    {
        auto& op = ops[iop];
        if (op.Active == 0)
            continue;

        r += R"(    if (1)
    {
        MLOP mop(&ml);
)";


		for (size_t inode = 0; inode < op.nodes.size(); inode++)
		{
			auto& node = op.nodes[inode];
			r += "\t\t";
			r += node->code;
			r += "\r\n";
		}
		r += R"(        ml.ops.push_back(mop.Build());
	}


)";
    }

	char buf[1000] = {};
    sprintf_s(buf,1000,R"(
		
	// Initialize
	ml.Prepare();

	// Upload data example
	// std::vector<float> data(100);
	// ml.ops[0].Item(0).buffer->Upload(&ml, data.data(), data.size() * sizeof(float));

    // Run the operators
	ml.Run();

	// Download data example
    // std::vector<char> cdata;
	// ml.ops[0].Item[ml.ops[0].Item.size() - 1].buffer->Download(&ml, 400, cdata);
    
)");
	r += buf;

    r += R"(})";
    return r;
}

