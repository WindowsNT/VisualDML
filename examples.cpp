#include "pch.h"
#include "MLGraph.xaml.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;


const std::vector<std::string> examples = {

	R"(		<Page n="">
			<Operators>
				<Operator DataType="1" Zoom="1.000000" Active="1">
					<Nodes>
						<Node Name="Input" xx="100.000000" yy="100.000000" CSV="" CSVI="&quot;Sequence&quot;" bf="0" sm="0" optype="1" Type="1">
							<Children>
								<Child idx="0" io="1" u="249"/>
							</Children>
							<Dimensions>
								<Dimension Size="10"/>
								<Dimension Size="10"/>
							</Dimensions>
						</Node>
						<Node Name="Input" xx="124.821533" yy="470.722656" CSV="" CSVI="&quot;Sequence&quot;" bf="0" sm="0" optype="1" Type="1">
							<Children>
								<Child idx="0" io="1" u="250"/>
							</Children>
							<Dimensions>
								<Dimension Size="10"/>
								<Dimension Size="10"/>
							</Dimensions>
						</Node>
						<Node xx="428.000000" yy="337.107422" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="2" no="1" Type="26" Name="Add">
							<Children>
								<Child idx="0" io="0" u="249"/>
								<Child idx="0" io="0" u="250"/>
								<Child idx="0" io="1" u="251"/>
							</Children>
						</Node>
						<Node xx="737.596680" yy="449.353516" CSV="&quot;Screen&quot;" CSVI="" bf="0" sm="0" optype="1" Name="Output" Type="999999">
							<Children>
								<Child idx="0" io="0" u="251"/>
							</Children>
						</Node>
					</Nodes>
				</Operator>
			</Operators>
		</Page>
)",

R"(		<Page n="">
			<Operators>
				<Operator DataType="1" Zoom="1.000000" Active="1">
					<Nodes>
						<Node Name="Input" xx="67.842773" yy="145.460938" CSV="" CSVI="&quot;Sequence&quot;" bf="0" sm="0" optype="1" Type="1">
							<Children>
								<Child idx="0" io="1" u="231,234"/>
							</Children>
							<Dimensions>
								<Dimension Size="1"/>
								<Dimension Size="10"/>
							</Dimensions>
						</Node>
						<Node Name="Input" xx="50.842773" yy="496.460938" CSV="" CSVI="&quot;Sequence&quot;" bf="0" sm="0" optype="1" Type="1">
							<Children>
								<Child idx="0" io="1" u="232"/>
							</Children>
							<Dimensions>
								<Dimension Size="1"/>
								<Dimension Size="1"/>
							</Dimensions>
						</Node>
						<Node xx="641.000000" yy="193.599609" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="1" no="1" Type="71" Name="Identity">
							<Children>
								<Child idx="0" io="0" u="231"/>
								<Child idx="0" io="1" u="221"/>
							</Children>
						</Node>
						<Node xx="284.065674" yy="621.460938" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="1" no="1" Type="100" Name="Reinterpret">
							<Children>
								<Child idx="0" io="0" u="232"/>
								<Child idx="0" io="1" u="233,237"/>
							</Children>
							<Params>
								<Param Name="New Size" Value="1x1x1x1" Min="-1.000000" Max="-1.000000"/>
							</Params>
						</Node>
						<Node xx="320.112549" yy="323.460938" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="1" no="1" Type="100" Name="Reinterpret">
							<Children>
								<Child idx="0" io="0" u="234"/>
								<Child idx="0" io="1" u="235,236"/>
							</Children>
							<Params>
								<Param Name="New Size" Value="1x1x1x10" Min="-1.000000" Max="-1.000000"/>
							</Params>
						</Node>
						<Node xx="627.353760" yy="555.830078" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="3" no="1" Type="67" Name="Gemm">
							<Children>
								<Child idx="0" io="0" u="236"/>
								<Child idx="0" io="0" u="237"/>
								<Child idx="0" io="0" u=""/>
								<Child idx="0" io="1" u="238"/>
							</Children>
							<Params>
								<Param Name="Transpose 1" Value="1" Min="0.000000" Max="1.000000"/>
								<Param Name="Transpose 2" Value="" Min="0.000000" Max="1.000000"/>
								<Param Name="Alpha" Value="1" Min="0.000000" Max="340282346638528859811704183484516925440.000000"/>
								<Param Name="Beta" Value="1" Min="0.000000" Max="340282346638528859811704183484516925440.000000"/>
							</Params>
						</Node>
						<Node xx="857.596680" yy="303.845703" CSV="&quot;Screen&quot;" CSVI="" bf="0" sm="0" optype="1" Name="Output" Type="999999">
							<Children>
								<Child idx="0" io="0" u="221"/>
							</Children>
						</Node>
						<Node xx="987.596680" yy="631.353516" CSV="&quot;Screen&quot;" CSVI="" bf="0" sm="0" optype="1" Name="Output" Type="999999">
							<Children>
								<Child idx="0" io="0" u="238"/>
							</Children>
						</Node>
					</Nodes>
				</Operator>
			</Operators>
		</Page>
)",

R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
		<Page n="">
			<Operators>
				<Operator DataType="1" Zoom="1.000000" Active="1">
					<Nodes>
						<Node xx="317.442383" yy="195.968750" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="0" no="1" Type="48" Name="Constant">
							<Children>
								<Child idx="0" io="1">
									<Connections>
										<Connection key="1">
											<Conditions>
												<Condition expression="jack &gt; 0"/>
											</Conditions>
										</Connection>
										<Connection key="4">
											<Conditions>
												<Condition expression="jack &lt;= 0"/>
											</Conditions>
										</Connection>
									</Connections>
								</Child>
							</Children>
							<Params>
								<Param Name="Value" Value="1.000000" Min="0.000000" Max="340282346638528859811704183484516925440.000000"/>
							</Params>
							<Dimensions>
								<Dimension Size="2"/>
								<Dimension Size="2"/>
							</Dimensions>
						</Node>
						<Node xx="342.442383" yy="598.968750" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="0" no="1" Type="48" Name="Constant">
							<Children>
								<Child idx="0" io="1">
									<Connections>
										<Connection key="2">
											<Conditions>
												<Condition expression="jack &lt;= 0"/>
											</Conditions>
										</Connection>
										<Connection key="11">
											<Conditions>
												<Condition expression="jack &gt; 0"/>
											</Conditions>
										</Connection>
									</Connections>
								</Child>
							</Children>
							<Params>
								<Param Name="Value" Value="2.000000" Min="0.000000" Max="340282346638528859811704183484516925440.000000"/>
							</Params>
							<Dimensions>
								<Dimension Size="2"/>
								<Dimension Size="2"/>
							</Dimensions>
						</Node>
						<Node xx="952.000000" yy="465.107422" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="1" no="1" Type="72" Name="Identity">
							<Children>
								<Child idx="0" io="0">
									<Connections>
										<Connection key="1">
											<Conditions />
										</Connection>
										<Connection key="2">
											<Conditions />
										</Connection>
									</Connections>
								</Child>
								<Child idx="0" io="1">
									<Connections>
										<Connection key="3">
											<Conditions />
										</Connection>
									</Connections>
								</Child>
							</Children>
						</Node>
						<Node xx="1269.596680" yy="412.599609" CSV="&quot;Screen&quot;" CSVI="" bf="0" sm="0" optype="1" ni="1" no="0" Type="999999" Name="Output">
							<Children>
								<Child idx="0" io="0">
									<Connections>
										<Connection key="3">
											<Conditions />
										</Connection>
									</Connections>
								</Child>
							</Children>
						</Node>
						<Node xx="1095.000000" yy="207.107422" CSV="" CSVI="" bf="0" sm="0" optype="1" ni="1" no="0" Type="999999" Name="Output">
							<Children>
								<Child idx="0" io="0">
									<Connections>
										<Connection key="4">
											<Conditions />
										</Connection>
										<Connection key="11">
											<Conditions />
										</Connection>
									</Connections>
								</Child>
							</Children>
						</Node>
					</Nodes>
				</Operator>
			</Operators>
			<Variables>
				<Variable n="jack" v="1"/>
			</Variables>
		</Page>
)"

};

namespace winrt::VisualDML::implementation
{
	void MLGraph::OnExample(const char* ee)
	{
		XML3::XML e;
		e = ee;
		XL xl;
		xl.Unser(e.GetRootElement());
		Push();
		prj.xl() = xl;
		FullRefresh();
	}

    void MLGraph::OnExample1(IInspectable, IInspectable)
    {
		OnExample(examples[0].c_str());
    }
	void MLGraph::OnExample2(IInspectable, IInspectable)
	{
		OnExample(examples[1].c_str());
	}
	void MLGraph::OnExample3(IInspectable, IInspectable)
	{
		OnExample(examples[2].c_str());
	}

}
