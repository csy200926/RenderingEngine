Actors = 
{
	{
		name = "Joe",
		class = "Player",
		collisionID = "Joe",
		collisionGroup = { "Inky" , Binky }
	} ,
	{
		name = "Inky",
		class = "Monster"
		collisionID = "Inky",
		collisionGroup = { Joe }
	} ,
	{
		name = "Binky",
		class = "Monster"
		collisionID = "Joe",
		collisionGroup = { Joe }
	} ,
	{
		name = "Clyde",
		class = "Monster"
		collisionID = "Clyde",
		collisionGroup = { Joe }
	}
}

ClassTypes = {
	"Monster",
	"Player",
	"Zombie",
	"Droid"
}