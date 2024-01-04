package main

import (
	"bo-dt/bo-dt-webapi/data"
	"net/http"

	"github.com/gin-gonic/gin"
)

var trees = data.GetTrees()

func getTrees(c *gin.Context) {
	c.IndentedJSON(http.StatusOK, trees)
}

func main() {
	router := gin.Default()
	router.GET("/trees", getTrees)

	router.Run("localhost:8080")

	data.GetTrees()
}
