using Godot;
using Moth.MicroMoth; // the published MicroMoth NuGet package
using System;

public partial class MicroMoth : Node
{
	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		GD.Print("Hello world"); // print onto the Godot console

		var bell = new QuantumCircuit(2, 2);
		bell.H(0).Cx(0, 1).MeasureAll(); // Create a simple Phi-plus bell state.
		// bell.H(0); // These lines work the same.
		// bell.Cx(0, 1);
		// bell.MeasureAll();
		GD.Print("Counts: ");
		foreach (var kv in Simulator.Counts(bell))
		{
			GD.Print($"{kv.Key}: {kv.Value}");
		}

		var probs = Simulator.Probabilities(bell);
		GD.Print($"Probabilities: P(00)={probs["00"]:F3} P(11)={probs["11"]:F3}");		
	}

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{
		
	}
}
